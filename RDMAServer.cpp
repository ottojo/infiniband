//
// Created by jonas on 22.05.21.
//

#include <stdexcept>
#include <fmt/format.h>
#include <utility>
#include <netdb.h>
#include "RDMAServer.hpp"
#include "rdmaLib.hpp"
#include <sys/eventfd.h>

RDMAServer::RDMAServer(int port, std::size_t sendBufferSize, std::size_t recvBufferSize,
                       std::function<void()> onConnect,
                       std::function<void(Buffer<char> &&b)> onReceive) :
        BufferSet(sendBufferSize, recvBufferSize),
        connectCallback(std::move(onConnect)),
        receiveCallback(std::move(onReceive)),
        ec(rdma_create_event_channel()) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    rdma_create_id(ec, &listenerConn, nullptr, RDMA_PS_TCP);
    rdma_bind_addr(listenerConn, (sockaddr *) &addr);
    rdma_listen(listenerConn, 10); // Arbitrary backlog 10
    fmt::print("Created listener rdma_cm_id {}\n", (void *) listenerConn);

    uint16_t portRes = ntohs(rdma_get_src_port(listenerConn));
    fmt::print("Listening on port {}\n", portRes);

    eventLoop = std::make_unique<BreakableEventLoop>(ec, [this](const rdma_cm_event &e) { on_event(e); });

    fmt::print("Server init done\n");
}

RDMAServer::~RDMAServer() {
    fmt::print("Stopping RDMAServer\n");

    if (clientConn != nullptr) {
        fmt::print("Disconnecting client\n");
        rdma_disconnect(clientConn);
        fmt::print("Destroying client connection id\n");
        rdma_destroy_id(clientConn);
    }

    fmt::print("Disconnecting listner\n");
    rdma_disconnect(listenerConn);

    fmt::print("Destroying listener ID\n");
    rdma_destroy_id(listenerConn);

    eventLoop.reset();

    fmt::print("Destroying event channel\n");
    rdma_destroy_event_channel(ec);

    ibv_dealloc_pd(protectionDomain);

    fmt::print("Stopped. Bye!\n");
}

bool RDMAServer::on_event(const rdma_cm_event &event) {
    fmt::print("Received event {} from rdma_cm_id {}\n", rdma_event_str(event.event), (void *) event.id);
    switch (event.event) {
        case RDMA_CM_EVENT_CONNECT_REQUEST:
            Expects(event.listen_id == listenerConn);
            return on_connect_request(event.id);
        case RDMA_CM_EVENT_ESTABLISHED:
            Expects(event.id == clientConn);
            return on_connection();
        case RDMA_CM_EVENT_DISCONNECTED:
            Expects(event.id == clientConn);
            return on_disconnect();
        default:
            throw std::runtime_error{fmt::format("Unknown event: {} ({})", event.event, rdma_event_str(event.event))};
    }
}

bool RDMAServer::on_connect_request(gsl::owner<rdma_cm_id *> newConn) {
    fmt::print("Received connect request\n");
    fmt::print("Building completion queue etc\n");

    if (clientConn != nullptr) {
        throw std::runtime_error{"Connection already exists"}; // TODO: multiple connections
    } else {
        clientConn = newConn;
        fmt::print("Creating completion poller for IB context of rdma_cm_id {}\n", (void *) clientConn);
        completionPoller = std::make_unique<CompletionPoller>(clientConn->verbs,
                                                              [this](const auto &wc) { on_completion(wc); });
    }

    fmt::print("Creating protection domain for connection {}", (void *) clientConn);
    protectionDomain = ibv_alloc_pd(
            clientConn->verbs); // TODO: Clear buffers associated with old PD when overwriting, as those cant be used with new client
    clearBuffers();

    fmt::print("Creating queue pair for rdma_cm_id {}\n", (void *) clientConn);
    ibv_qp_init_attr qp_attr = build_qp_attr(completionPoller->cq);
    if (rdma_create_qp(clientConn, protectionDomain, &qp_attr) != 0) {
        throw std::runtime_error{fmt::format("Error creating queue pair: {}", strerror(errno))};
    }
    post_receives();
    rdma_conn_param cm_params{};
    fmt::print("Accepting connection {}\n", (void *) clientConn);
    if (rdma_accept(clientConn, &cm_params) != 0) {
        throw std::runtime_error{fmt::format("Error accepting connection: {}", strerror(errno))};
    }
    return false;
}

bool RDMAServer::on_disconnect() {
    fmt::print("peer disconnected\n");
    completionPoller.reset();
    ibv_dealloc_pd(protectionDomain);
    fmt::print("Destroy RDMA communication identifier\n");
    rdma_destroy_id(clientConn); // TODO: Pass client connection via parameter
    clientConn = nullptr;
    return false;
}

bool RDMAServer::on_connection() {
    connectCallback();
    return false;
}


void RDMAServer::on_completion(const ibv_wc &wc) {
    fmt::print("Received work completion with opcode {}\n", wc.opcode);
    if (wc.status != IBV_WC_SUCCESS) {
        throw std::runtime_error{fmt::format("on_completion: status is not success: {}", ibv_wc_status_str(wc.status))};
    }
    char *receiveBufferData = reinterpret_cast<char *>(wc.wr_id);

    if (wc.opcode == IBV_WC_RECV) {
        fmt::print("Received message, calling callback\n");
        receiveCallback(findReceiveBuffer(receiveBufferData));
    } else if (wc.opcode == IBV_WC_SEND) {
        fmt::print("Send of wr {} completed successfully\n", wc.wr_id);
        returnBuffer(findSendBuffer(receiveBufferData));
    }
}

void RDMAServer::post_receives() {
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
    fmt::print("Posting receive with size {} to rdma_cm_id {}\n", sge.length, (void *) clientConn);
    ibv_post_recv(clientConn->qp, &wr, &bad_wr);;
    markInFlightRecv(std::move(b));
}

void RDMAServer::send(Buffer<char> &&b) {
    assert(b.size() == getSendSize());

    ibv_sge sge{};
    sge.addr = reinterpret_cast<uint64_t>(b.data());
    sge.length = b.size();
    sge.lkey = b.getMR()->lkey;

    ibv_send_wr wr{};
    wr.opcode = IBV_WR_SEND; // Send request that must match a corresponding receive request on the peer
    wr.wr_id = reinterpret_cast<uint64_t>(b.data());
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED; // We want complete notification for this send request

    ibv_send_wr *bad_wr = nullptr;
    fmt::print("Posting send request for result\n");
    ibv_post_send(clientConn->qp, &wr, &bad_wr);
    markInFlightSend(std::move(b));
}

Buffer<char> RDMAServer::getSendBuffer() {
    // TODO: Handle multiple buffers for PDs
    return BufferSet::getSendBuffer().value_or(Buffer<char>(getSendSize(), protectionDomain));
}
