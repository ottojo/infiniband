/**
 * @file RDMAClient.cpp
 * @author ottojo
 * @date 5/24/21
 * Description here TODO
 */

#include "RDMAClient.hpp"

#include <fmt/format.h>

#include <utility>
#include <netdb.h>
#include <cassert>
#include <optional>
//#include <opencv2/opencv.hpp>

RDMAClient::RDMAClient(std::string server, std::string port, std::size_t sendBufferSize, std::size_t recvBufferSize,
                       ReceiveCallback receiveCallback) :
        recv_size(recvBufferSize),
        send_size(sendBufferSize),
        receiveCallback(std::move(receiveCallback)) {
    ec = rdma_create_event_channel();
    if (ec == nullptr) {
        throw std::runtime_error{fmt::format("Error creating event channel: {}", strerror(errno))};
    }
    rdma_cm_id *tempRdmaId = nullptr;
    rdma_create_id(ec, &tempRdmaId, nullptr, RDMA_PS_TCP);
    if (tempRdmaId == nullptr) {
        throw std::runtime_error{fmt::format("Error creating communication identifier: {}", strerror(errno))};
    }

    eventLoop = std::make_unique<BreakableEventLoop>(ec, [this](const rdma_cm_event &event) {
        on_event(event);
    });

    addrinfo *addr;
    if (getaddrinfo(server.c_str(), port.c_str(), nullptr, &addr) != 0) {
        throw std::runtime_error{"getaddrinfo failed"};
    }
    // Bind conn to address
    if (rdma_resolve_addr(tempRdmaId, nullptr, addr->ai_addr, 500 /* ms timeout */) == -1) {
        throw std::runtime_error{fmt::format("Error resolving address: {}", strerror(errno))};

    }
    freeaddrinfo(addr);

    // Address resolved -> route resolved -> connected
    auto connected = connectedPromise.get_future();
    connected.wait();
}

RDMAClient::~RDMAClient() {
    rdma_disconnect(conn->id);

    rdma_destroy_event_channel(ec);
// TODO think about order
    eventLoop.reset();
}


void RDMAClient::on_completion(const ibv_wc &wc) {
    if (wc.status != IBV_WC_SUCCESS) {
        throw std::runtime_error{fmt::format("Received work completion with status \"{}\" ({}), which is not success.",
                                             ibv_wc_status_str(wc.status), wc.status)};
    }

    if (wc.opcode == IBV_WC_RECV) {
        // Incoming message is in buffer, move buffer back to user
        auto buffer = std::move(inFlightReceiveBuffers.at(reinterpret_cast<char *>(wc.wr_id)));
        inFlightReceiveBuffers.erase(reinterpret_cast<char *>(wc.wr_id));
        receiveCallback(wc, std::move(buffer));
    } else if (wc.opcode == IBV_WC_SEND) {
        fmt::print("Send completed successfully\n");
    }
}

bool RDMAClient::on_event(const rdma_cm_event &event) {
    switch (event.event) {
        case RDMA_CM_EVENT_ADDR_RESOLVED:
            return on_address_resolved(event.id);
        case RDMA_CM_EVENT_ROUTE_RESOLVED:
            return on_route_resolved(event.id);
        case RDMA_CM_EVENT_ESTABLISHED: {
            auto ret = on_connection();
            connectedPromise.set_value(ret);
            return ret;
        }
        case RDMA_CM_EVENT_DISCONNECTED:
            return on_disconnect(event.id);
        default:
            throw std::runtime_error{fmt::format("Unknown event: {} ({})", event.event, rdma_event_str(event.event))};
    }
}

bool RDMAClient::on_route_resolved(rdma_cm_id *id) {
    fmt::print("Route resolved.\n");
    // Connect to server
    rdma_conn_param cm_param{};
    rdma_connect(id, &cm_param);
    return false;
}

bool RDMAClient::on_address_resolved(rdma_cm_id *id) {
    fmt::print("Address resolved.\n");
    // Now we have a valid verbs Context and can initialize the global context (PD, CQ, CQ poller thread)

    assert(global_ctx == nullptr); // This should only happen once during initialization
    global_ctx = std::make_unique<Context>(id->verbs, [this](const ibv_wc &wc) { on_completion(wc); });

    ibv_qp_init_attr qp_attr = build_qp_attr(global_ctx.get());
    rdma_create_qp(id, global_ctx->pd, &qp_attr);
    // Store id and QueuePair in id (used in on_disconnect)
    conn = new ClientConnection();
    id->context = conn;
    conn->id = id;
    conn->qp = id->qp;

    post_receives();

    rdma_resolve_route(id, 500 /* ms */);
    return false;
}

bool RDMAClient::on_disconnect(rdma_cm_id *id) {
    fmt::print("disconnected\n");

    auto *conn = static_cast<ClientConnection *>(id->context);

    assert(conn->id == id);

    rdma_destroy_qp(id);


    delete conn;
    rdma_destroy_id(id);
    return true;
}

bool RDMAClient::on_connection() {
    fmt::print("connected.\n");
    return false;
}

void RDMAClient::post_receives() {

    std::optional<Buffer<char>> b;
    if (not receiveBufferPool.empty()) {
        b = std::move(receiveBufferPool.front());
        receiveBufferPool.pop();
    } else {
        b.emplace(send_size, global_ctx->pd);
    }

    ibv_sge sge{};
    sge.addr = reinterpret_cast<uint64_t >(b->data());
    sge.length = recv_size;
    sge.lkey = b->getMR()->lkey;

    ibv_recv_wr wr{};
    wr.wr_id = reinterpret_cast<uintptr_t>(b->data()); // Identify the buffer by the data pointer once WR completed
    wr.next = nullptr;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    ibv_recv_wr *bad_wr = nullptr;
    ibv_post_recv(conn->qp, &wr, &bad_wr);
    inFlightReceiveBuffers.emplace(b->data(), std::move(b.value()));
}

Buffer<char> RDMAClient::getBuffer() {
    // TODO: mempool
    return Buffer<char>(send_size, global_ctx->pd);
}

void RDMAClient::send(Buffer<char> &&b) {
    fmt::print("Sending\n");

    ibv_sge sge{};
    sge.addr = reinterpret_cast<uint64_t>(b.getMR());
    sge.length = send_size;
    sge.lkey = b.getMR()->lkey;

    ibv_send_wr wr{};
    wr.opcode = IBV_WR_SEND; // Send request that must match a corresponding receive request on the peer
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED; // We want complete notification for this send request

    ibv_send_wr *bad_wr = nullptr;
    fmt::print("posting send...\n");
    ibv_post_send(conn->qp, &wr, &bad_wr);

    inFlightSendBuffers.emplace(b.data(), std::move(b));
}

void RDMAClient::returnBuffer(Buffer<char> &&b) {
    if (b.size() == recv_size) {
        receiveBufferPool.emplace(std::move(b));
    } else if (b.size() == send_size) {
        sendBufferPool.emplace(std::move(b));
    } else {
        throw std::runtime_error{
                fmt::format("Can not return buffer of size {}, we only have pools for sizes {} and {}", b.size(),
                            recv_size, send_size)};
    }
}
