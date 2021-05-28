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

RDMAClient::RDMAClient(const std::string &server, const std::string &port, std::size_t sendBufferSize,
                       std::size_t recvBufferSize,
                       ReceiveCallback receiveCallback) :
        BufferSet(sendBufferSize, recvBufferSize),
        receiveCallback(std::move(receiveCallback)),
        ec(rdma_create_event_channel()) {
    if (ec == nullptr) {
        throw std::runtime_error{fmt::format("Error creating event channel: {}", strerror(errno))};
    }

    if (rdma_create_id(ec, &conn, nullptr, RDMA_PS_TCP) == -1 or conn == nullptr) {
        throw std::runtime_error{fmt::format("Error creating communication identifier: {}", strerror(errno))};
    }

    eventLoop = std::make_unique<BreakableEventLoop>(ec, [this](const rdma_cm_event &event) {
        on_event(event);
    });

    addrinfo *addr;
    if (getaddrinfo(server.c_str(), port.c_str(), nullptr, &addr) != 0) {
        throw std::runtime_error{"getaddrinfo failed"};
    }
    // Bind conn to address, generates RDMA_CM_EVENT_ADDR_RESOLVED
    if (rdma_resolve_addr(conn, nullptr, addr->ai_addr, 500 /* ms timeout */) == -1) {
        throw std::runtime_error{fmt::format("Error resolving address: {}", strerror(errno))};
    }
    freeaddrinfo(addr);

    // Address resolved -> route resolved -> connected
    auto connected = connectedPromise.get_future();
    connected.wait();
}

RDMAClient::~RDMAClient() {
    // TODO: handle disconnect during lifetime

    // Disconnect: Transition QP to error state, flushes any posted WRs to completion queue.
    // Generates RDMA_CM_EVENT_DISCONNECTED.
    if (rdma_disconnect(conn) == -1) {
        fmt::print("Error while disconnecting: {}\n", strerror(errno));
    }


    ibv_dealloc_pd(protectionDomain);

    eventLoop.reset();
    rdma_destroy_id(conn);
    rdma_destroy_event_channel(ec);
}


void RDMAClient::on_completion(const ibv_wc &wc) {
    if (wc.status != IBV_WC_SUCCESS) {
        throw std::runtime_error{fmt::format("Received work completion with status \"{}\" ({}), which is not success.",
                                             ibv_wc_status_str(wc.status), wc.status)};
    }
    char *receiveBufferData = reinterpret_cast<char *>(wc.wr_id);
    if (wc.opcode == IBV_WC_RECV) {
        // Incoming message is in buffer, move buffer back to user
        auto buffer = findReceiveBuffer(receiveBufferData);
        receiveCallback(wc, std::move(buffer));
    } else if (wc.opcode == IBV_WC_SEND) {
        fmt::print("Send completed successfully\n");
        returnBuffer(findSendBuffer(receiveBufferData));
    }
}

bool RDMAClient::on_event(const rdma_cm_event &event) {
    switch (event.event) {
        case RDMA_CM_EVENT_ADDR_RESOLVED:
            //
            return on_address_resolved();
        case RDMA_CM_EVENT_ROUTE_RESOLVED:
            return on_route_resolved(event.id);
        case RDMA_CM_EVENT_ESTABLISHED: {
            auto ret = on_connection();
            connectedPromise.set_value(ret);
            return ret;
        }
        case RDMA_CM_EVENT_DISCONNECTED:
            return on_disconnect();
        default:
            throw std::runtime_error{fmt::format("Unknown event: {} ({})", event.event, rdma_event_str(event.event))};
    }
}

bool RDMAClient::on_address_resolved() {
    fmt::print("Address resolved.\n");
    // Now we have a valid verbs Context and can initialize the global context (PD, CQ, CQ poller thread)
    assert(completionPoller == nullptr); // This should only happen once during initialization
    completionPoller = std::make_unique<CompletionPoller>(conn->verbs, [this](const ibv_wc &wc) { on_completion(wc); });


    fmt::print("Allocating protection domain\n");
    protectionDomain = ibv_alloc_pd(conn->verbs);
    if (protectionDomain == nullptr) {
        throw std::runtime_error{fmt::format("Error allocating protection domain: {}", strerror(errno))};
    }

    fmt::print("Creating queue pair\n");
    ibv_qp_init_attr qp_attr = build_qp_attr(completionPoller->cq);
    rdma_create_qp(conn, protectionDomain, &qp_attr);

    post_receives();

    fmt::print("Resolving route\n");
    rdma_resolve_route(conn, 500 /* ms */);
    return false;
}

bool RDMAClient::on_route_resolved(rdma_cm_id *id) {
    fmt::print("Route resolved\n");
    // Connect to server
    rdma_conn_param cm_param{};
    rdma_connect(id, &cm_param);
    return false;
}

bool RDMAClient::on_connection() {
    fmt::print("connected\n");
    return false;
}

bool RDMAClient::on_disconnect() {
    fmt::print("disconnected\n");

    rdma_destroy_qp(conn);
    rdma_destroy_id(conn);
    return true;
}

void RDMAClient::post_receives() {
    fmt::print("Posting receives\n");

    Buffer<char> b = getRecvBuffer().value_or(Buffer<char>(getSendSize(), protectionDomain));

    ibv_sge sge{};
    sge.addr = reinterpret_cast<uint64_t >(b.data());
    sge.length = getRecvSize();
    sge.lkey = b.getMR()->lkey;

    ibv_recv_wr wr{};
    wr.wr_id = reinterpret_cast<uintptr_t>(b.data()); // Identify the buffer by the data pointer once WR completed
    wr.next = nullptr;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    ibv_recv_wr *bad_wr = nullptr;
    ibv_post_recv(conn->qp, &wr, &bad_wr);;
    markInFlightRecv(std::move(b));
}

Buffer<char> RDMAClient::getSendBuffer() {
    return BufferSet::getSendBuffer().value_or(Buffer<char>(getSendSize(), protectionDomain));
}

void RDMAClient::send(Buffer<char> &&b) {
    fmt::print("Sending\n");

    ibv_sge sge{};
    sge.addr = reinterpret_cast<uint64_t>(b.getMR());
    sge.length = getSendSize();
    sge.lkey = b.getMR()->lkey;

    ibv_send_wr wr{};
    wr.opcode = IBV_WR_SEND; // Send request that must match a corresponding receive request on the peer
    wr.wr_id = reinterpret_cast<uint64_t>(b.data());
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED; // We want complete notification for this send request

    ibv_send_wr *bad_wr = nullptr;
    fmt::print("posting send with size {}\n", sge.length);
    ibv_post_send(conn->qp, &wr, &bad_wr);

    markInFlightSend(std::move(b));
}

