//
// Created by jonas on 08.04.21.
//

#include <fmt/format.h>
#include <rdma/rdma_cma.h>
#include <thread>
#include <unistd.h>
#include <netdb.h>

constexpr auto BUFFER_SIZE = 1024;

struct connection {
    rdma_cm_id *id;
    ibv_qp *qp;
    ibv_mr *send_mr;
    ibv_mr *recv_mr;
    char *recv_region;
    char *send_region;
    int num_completions;
};

struct context {
    ibv_context *ctx;
    ibv_pd *pd;
    ibv_cq *cq;
    ibv_comp_channel *comp_channel;

    std::thread cq_poller_thread;
};

context *s_ctx = nullptr;

void on_completion(ibv_wc *wc) {
    if (wc->status != IBV_WC_SUCCESS) {
        throw std::runtime_error{"on_complation: status is not success"};
    }

    auto *conn = reinterpret_cast<connection *>(wc->wr_id);
    if (wc->opcode == IBV_WC_RECV) { // TODO: Tutorial tests with &, but this is an enum and not binary flags?
        fmt::print("Received message: \"{}\"\n", conn->recv_region);
    } else if (wc->opcode == IBV_WC_SEND) {
        fmt::print("Send completed successfully\n");
    }
    conn->num_completions++;
    if (conn->num_completions >= 2) {
        rdma_disconnect(conn->id);
    }
}

[[noreturn]] void poll_cq() {
    ibv_cq *cq;
    void *ctx = nullptr;
    while (true) {
        ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx);
        ibv_ack_cq_events(cq, 1);
        ibv_req_notify_cq(cq, 0);
        ibv_wc wc{};
        while (ibv_poll_cq(cq, 1, &wc)) {
            on_completion(&wc);
        }
    }
}

void build_context(ibv_context *verbs) {
    if (s_ctx != nullptr) {
        if (s_ctx->ctx != verbs) {
            throw std::runtime_error{"cannot handle events in more than one context"};
        }
        fmt::print("Context already exists!\n");
        return;
    }
    s_ctx = static_cast<context *>(malloc(sizeof(context)));
    s_ctx->ctx = verbs;
    s_ctx->pd = ibv_alloc_pd(s_ctx->ctx);
    s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx);
    s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, nullptr, s_ctx->comp_channel, 0);
    ibv_req_notify_cq(s_ctx->cq, 0);

    s_ctx->cq_poller_thread = std::thread([]() {
        poll_cq();
    });
}

ibv_qp_init_attr build_qp_attr(context *ctx) {
    ibv_qp_init_attr qp_attr{};
    qp_attr.send_cq = ctx->cq;
    qp_attr.recv_cq = ctx->cq;
    qp_attr.qp_type = IBV_QPT_RC;

    qp_attr.cap.max_send_wr = 10;
    qp_attr.cap.max_recv_wr = 10;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;
    return qp_attr;
}

void register_memory(connection *conn) {
    conn->send_region = static_cast<char *>(malloc(BUFFER_SIZE));
    conn->recv_region = static_cast<char *>(malloc(BUFFER_SIZE));

    conn->send_mr = ibv_reg_mr(s_ctx->pd,
                               conn->send_region,
                               BUFFER_SIZE,
                               IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);

    conn->recv_mr = ibv_reg_mr(s_ctx->pd,
                               conn->recv_region,
                               BUFFER_SIZE,
                               IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
}

void post_receives(connection *conn) {
    ibv_sge sge{};
    sge.addr = reinterpret_cast<uintptr_t>(conn->recv_region);
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


bool on_connection(void *context) {
    auto *conn = static_cast<connection *>(context);


    fmt::format_to_n(conn->send_region, BUFFER_SIZE, "message from active/client side with pid {}", getpid());

    fmt::print("connected. posting send...\n");

    ibv_sge sge{};
    sge.addr = reinterpret_cast<uint64_t>(conn->send_region);
    sge.length = BUFFER_SIZE;
    sge.lkey = conn->send_mr->lkey;

    ibv_send_wr wr{};
    wr.wr_id = reinterpret_cast<uintptr_t>(conn);
    wr.opcode = IBV_WR_SEND; // Send request that must match a corresponding receive request on the peer
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED; // We want complete notification for this send request

    ibv_send_wr *bad_wr = nullptr;
    ibv_post_send(conn->qp, &wr, &bad_wr);
    return false;
}

bool on_disconnect(rdma_cm_id *id) {
    fmt::print("disconnected\n");

    auto *conn = static_cast<connection *>(id->context);
    rdma_destroy_qp(id);
    ibv_dereg_mr(conn->send_mr);
    ibv_dereg_mr(conn->recv_mr);
    free(conn->send_region);
    free(conn->recv_mr);
    free(conn);
    rdma_destroy_id(id);
    return true;
}

bool on_address_resolved(rdma_cm_id *id) {
    fmt::print("Address resolved.\n");
    // Now we have a valid verbs context and can initialize the connection
    build_context(id->verbs);
    ibv_qp_init_attr qp_attr = build_qp_attr(s_ctx);
    rdma_create_qp(id, s_ctx->pd, &qp_attr);
    auto *conn = static_cast<connection *>(malloc(sizeof(connection)));
    id->context = conn;
    conn->id = id;
    conn->qp = id->qp;
    conn->num_completions = 0;

    register_memory(conn);
    post_receives(conn);

    rdma_resolve_route(id, 500 /* ms */);
    return false;
}

bool on_route_resolved(rdma_cm_id *id) {
    fmt::print("Route resolved.\n");
    // Connect to server
    rdma_conn_param cm_param{};
    rdma_connect(id, &cm_param);
    return false;
}

/**
 * @return true to end event loop
 */
bool on_event(rdma_cm_event *event) {
    switch (event->event) {
        case RDMA_CM_EVENT_ADDR_RESOLVED:
            return on_address_resolved(event->id);
        case RDMA_CM_EVENT_ROUTE_RESOLVED:
            return on_route_resolved(event->id);
        case RDMA_CM_EVENT_ESTABLISHED:
            return on_connection(event->id->context);
        case RDMA_CM_EVENT_DISCONNECTED:
            return on_disconnect(event->id);
        default:
            throw std::runtime_error{"Unknown event"};
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fmt::print("Usage: {} <server-address> <server-port>\n", argv[0]);
        std::exit(1);
    }

    addrinfo *addr;
    if (getaddrinfo(argv[1], argv[2], nullptr, &addr) != 0) {
        throw std::runtime_error{"getaddrinfo failed"};
    }

    rdma_event_channel *ec = rdma_create_event_channel();


    rdma_cm_id *conn = nullptr;
    rdma_create_id(ec, &conn, nullptr, RDMA_PS_TCP); // TODO: Why TCP and not IB?

    rdma_resolve_addr(conn, nullptr, addr->ai_addr, 500 /* ms timeout */);
    freeaddrinfo(addr);

    rdma_cm_event *event = nullptr;
    while (rdma_get_cm_event(ec, &event) == 0) {
        rdma_cm_event event_copy = *event;
        rdma_ack_cm_event(event);
        if (on_event(&event_copy)) {
            break;
        }
    }
    rdma_destroy_event_channel(ec);
    return 0;
}
