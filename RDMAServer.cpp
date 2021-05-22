//
// Created by jonas on 22.05.21.
//

#include <stdexcept>
#include <fmt/format.h>
#include <utility>
#include "RDMAServer.hpp"
#include "rdmaLib.hpp"

RDMAServer::RDMAServer(int port, std::size_t sendBufferSize, std::size_t recvBufferSize,
                       std::function<void()> onConnect,
                       std::function<void(RDMAServer &server, ServerConnection &connection)> onReceive) :
        sendBufferSize(sendBufferSize),
        recvBufferSize(recvBufferSize),
        connectCallback(std::move(onConnect)),
        receiveCallback(std::move(onReceive)) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    ec = rdma_create_event_channel();

    rdma_create_id(ec, &listener, nullptr, RDMA_PS_TCP);
    rdma_bind_addr(listener, (sockaddr *) &addr);
    rdma_listen(listener, 10); // Arbitrary backlog 10

    uint16_t portRes = ntohs(rdma_get_src_port(listener));
    fmt::print("Listening on port {}\n", portRes);

    eventLoop = std::thread([this]() {
        rdma_cm_event *event = nullptr;

        while (rdma_get_cm_event(ec, &event) == 0) {
            rdma_cm_event event_copy = *event;
            rdma_ack_cm_event(event);
            if (on_event(&event_copy) or eventLoopEnd) {
                fmt::print("Event loop stopping.\n");
                break;
            }
        }
    });
    fmt::print("Server init done\n");
}

RDMAServer::~RDMAServer() {
    fmt::print("Stopping RDMAServer\n");
    eventLoopEnd = true;
    fmt::print("Waiting for connection manager event loop\n");
    eventLoop.join();
    rdma_destroy_id(listener);
    rdma_destroy_event_channel(ec);
    fmt::print("Stopped. Bye!\n");
}

bool RDMAServer::on_event(rdma_cm_event *event) {
    switch (event->event) {
        case RDMA_CM_EVENT_CONNECT_REQUEST:
            return on_connect_request(event->id);
        case RDMA_CM_EVENT_ESTABLISHED:
            return on_connection(event->id->context);
        case RDMA_CM_EVENT_DISCONNECTED:
            return on_disconnect(event->id);
        default:
            throw std::runtime_error{fmt::format("Unknown event: {} ({})", event->event, rdma_event_str(event->event))};
    }
}

bool RDMAServer::on_connect_request(rdma_cm_id *id) {
    fmt::print("Received connect request\n");
    build_context(id->verbs);
    ibv_qp_init_attr qp_attr = build_qp_attr();
    rdma_create_qp(id, s_ctx->pd, &qp_attr);
    auto *conn = new ServerConnection;
    id->context = conn;
    conn->qp = id->qp;
    register_memory(conn);
    post_receives(conn);
    rdma_conn_param cm_params{};
    rdma_accept(id, &cm_params);
    return false;
}

bool RDMAServer::on_disconnect(rdma_cm_id *id) {
    fmt::print("peer disconnected\n");

    auto *conn = static_cast<ServerConnection *>(id->context);
    fmt::print("Destroying queue pair\n");
    rdma_destroy_qp(id);
    fmt::print("Removing memory regions\n");
    ibv_dereg_mr(conn->send_mr);
    ibv_dereg_mr(conn->recv_mr);
    fmt::print("Deleting ServerConnection (including send/recv buffers)\n");
    delete conn;
    fmt::print("Destroy RDMA communication identifier\n");
    rdma_destroy_id(id);
    return false;
}

bool RDMAServer::on_connection(void *context) {
    connectCallback();
    return false;
}

void RDMAServer::build_context(ibv_context *verbsContext) {
    fmt::print("Building context from ibv context\n");
    if (s_ctx != nullptr) {
        if (s_ctx->ctx != verbsContext) {
            throw std::runtime_error{"Context with different ibv context already exists"};
        }
        fmt::print("Context already exists!\n");
        return;
    }
    s_ctx = std::make_unique<Context>();
    s_ctx->ctx = verbsContext;
    fmt::print("Allocating protection domain\n");
    s_ctx->pd = ibv_alloc_pd(s_ctx->ctx);
    fmt::print("Creating completion channel\n");
    s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx);
    fmt::print("Creating completion queue\n");
    s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, nullptr, s_ctx->comp_channel, 0);
    // Request notification for next event, even if not solicited
    ibv_req_notify_cq(s_ctx->cq, 0);

    s_ctx->cq_poller_thread = std::thread([this]() {
        void *ctx = nullptr;
        while (true) {
            ibv_cq *cq;
            // Wait for next completion event in the channel
            ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx);
            // Acknowledge completion event
            ibv_ack_cq_events(cq, 1);
            // Request a notification when the next Work Completion is added to the Completion Queue
            ibv_req_notify_cq(cq, 0);
            // Empty the completion queue of WCs
            ibv_wc wc{};
            while (ibv_poll_cq(cq, 1, &wc)) {
                on_completion(wc);
            }
        }
    });
}

void RDMAServer::on_completion(const ibv_wc &wc) {
    if (wc.status != IBV_WC_SUCCESS) {
        throw std::runtime_error{fmt::format("on_completion: status is not success: {}", ibv_wc_status_str(wc.status))};
    }
    if (wc.opcode == IBV_WC_RECV) {
        auto *conn = reinterpret_cast<ServerConnection *>(wc.wr_id);
        fmt::print("Received message, calling callback\n");
        receiveCallback(*this, *conn);
    } else if (wc.opcode == IBV_WC_SEND) {
        fmt::print("Send of wr {} completed successfully\n", wc.wr_id);
    }
}

void RDMAServer::register_memory(ServerConnection *conn) {
    conn->send_region = std::vector<char>(sendBufferSize, 0);
    conn->recv_region = std::vector<char>(recvBufferSize, 0);

    conn->send_mr = ibv_reg_mr(s_ctx->pd,
                               conn->send_region.data(),
                               conn->send_region.size(),
                               IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);

    conn->recv_mr = ibv_reg_mr(s_ctx->pd,
                               conn->recv_region.data(),
                               conn->recv_region.size(),
                               IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
}

void RDMAServer::post_receives(ServerConnection *conn) {
    ibv_sge sge{};
    sge.addr = reinterpret_cast<uintptr_t>(conn->recv_region.data());
    sge.length = BUFFER_SIZE;
    sge.lkey = conn->recv_mr->lkey;

    ibv_recv_wr wr{};
    wr.wr_id = reinterpret_cast<uintptr_t>(conn);
    wr.next = nullptr;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    ibv_recv_wr *bad_wr = nullptr;
    ibv_post_recv(conn->qp, &wr, &bad_wr);
}

ibv_qp_init_attr RDMAServer::build_qp_attr() {
    ibv_qp_init_attr qp_attr{};
    qp_attr.send_cq = s_ctx->cq;
    qp_attr.recv_cq = s_ctx->cq;
    qp_attr.qp_type = IBV_QPT_RC;

    qp_attr.cap.max_send_wr = 10;
    qp_attr.cap.max_recv_wr = 10;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;
    return qp_attr;
}

void RDMAServer::send(ServerConnection &conn) {
    ibv_sge sge{};
    sge.addr = reinterpret_cast<uint64_t>(conn.send_region.data());
    sge.length = conn.send_region.size();
    sge.lkey = conn.send_mr->lkey;

    ibv_send_wr wr{};
    wr.opcode = IBV_WR_SEND; // Send request that must match a corresponding receive request on the peer
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED; // We want complete notification for this send request

    ibv_send_wr *bad_wr = nullptr;
    fmt::print("Posting send request for result\n");
    ibv_post_send(conn.qp, &wr, &bad_wr);
}
