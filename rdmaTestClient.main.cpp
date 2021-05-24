//
// Created by jonas on 08.04.21.
//

#include <fmt/format.h>

#include <rdma/rdma_cma.h>
#include <thread>
#include <netdb.h>
#include <opencv2/opencv.hpp>

#include "rdmaLib.hpp"

std::chrono::high_resolution_clock::time_point sendTime;

Context *global_ctx = nullptr;

void on_completion(const ibv_wc &wc) {
    if (wc.status != IBV_WC_SUCCESS) {
        throw std::runtime_error{fmt::format("Received work completion with status \"{}\" ({}), which is not success.",
                                             ibv_wc_status_str(wc.status), wc.status)};
    }

    auto *conn = reinterpret_cast<ClientConnection *>(wc.wr_id);
    if (wc.opcode == IBV_WC_RECV) { // TODO: Tutorial tests with &, but this is an enum and not binary flags?

        auto time = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - sendTime);
        fmt::print("Received message after {}ms\n", time.count());

        cv::Mat inputMatrix(HEIGHT, WIDTH, CV_8UC1, (void *) conn->recv_region);

        cv::imshow("circle", inputMatrix);
        while (cv::waitKey(1) != 27);


    } else if (wc.opcode == IBV_WC_SEND) {
        fmt::print("Send completed successfully\n");
    }
    conn->num_completions++;
    if (conn->num_completions >= 2) {
        rdma_disconnect(conn->id);
    }
}

[[noreturn]] void poll_cq(Context *s_ctx) {
    ibv_cq *cq;
    void *ctx = nullptr;
    while (true) {
        ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx);
        ibv_ack_cq_events(cq, 1);
        ibv_req_notify_cq(cq, 0);
        ibv_wc wc{};
        while (ibv_poll_cq(cq, 1, &wc)) {
            on_completion(wc);
        }
    }
}

Context *build_context(ibv_context *verbs) {
    auto *s_ctx = new Context();// static_cast<Context *>(calloc(1, sizeof(Context)));
    s_ctx->ctx = verbs;
    s_ctx->pd = ibv_alloc_pd(s_ctx->ctx);
    s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx);
    s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, nullptr, s_ctx->comp_channel, 0);
    ibv_req_notify_cq(s_ctx->cq, 0);

    assert(not s_ctx->cq_poller_thread.joinable());

    s_ctx->cq_poller_thread = std::thread([s_ctx]() {
        poll_cq(s_ctx);
    });
    return s_ctx;
}


void register_memory(ClientConnection *conn, ibv_pd *pd) {
    fmt::print("Allocating send and receive buffers\n");
    conn->send_region = new char[BUFFER_SIZE];
    conn->recv_region = new char[BUFFER_SIZE];

    fmt::print("Registering memory regions with protection domain\n");
    conn->send_mr = ibv_reg_mr(pd,
                               conn->send_region,
                               BUFFER_SIZE,
                               IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
    if (conn->send_mr == nullptr) {
        throw std::runtime_error{fmt::format("Registering send memory region failed: {}", strerror(errno))};
    }

    conn->recv_mr = ibv_reg_mr(pd,
                               conn->recv_region,
                               BUFFER_SIZE,
                               IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
    if (conn->recv_mr == nullptr) {
        throw std::runtime_error{fmt::format("Registering receive memory region failed.")};
    }
}

void post_receives(ClientConnection *conn) {
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
    fmt::print("connected.\n");
    auto *conn = static_cast<ClientConnection *>(context);

    cv::Mat sendMatrix(HEIGHT, WIDTH, CV_8UC1, (void *) conn->send_region);
    sendMatrix.setTo(cv::Scalar(255));
    cv::circle(sendMatrix, cv::Point(WIDTH / 2, HEIGHT / 2), HEIGHT / 2, cv::Scalar(0), 20);

    fmt::print("Sending\n", *(conn->send_region));

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
    fmt::print("posting send...\n");
    sendTime = std::chrono::high_resolution_clock::now();
    ibv_post_send(conn->qp, &wr, &bad_wr);
    return false;
}

bool on_disconnect(rdma_cm_id *id) {
    fmt::print("disconnected\n");

    auto *conn = static_cast<ClientConnection *>(id->context);
    rdma_destroy_qp(id);
    ibv_dereg_mr(conn->send_mr);
    ibv_dereg_mr(conn->recv_mr);
    delete[] conn->send_region;
    delete[] conn->recv_region;
    delete conn;
    rdma_destroy_id(id);
    return true;
}

bool on_address_resolved(rdma_cm_id *id) {
    fmt::print("Address resolved.\n");
    // Now we have a valid verbs Context and can initialize the ClientConnection

    if (global_ctx != nullptr) {
        if (global_ctx->ctx != id->verbs) {
            throw std::runtime_error{"cannot handle events in more than one Context"};
        }
        fmt::print("Context already exists!\n");
    } else {
        global_ctx = build_context(id->verbs);
    }


    ibv_qp_init_attr qp_attr = build_qp_attr(global_ctx);
    rdma_create_qp(id, global_ctx->pd, &qp_attr);
    auto *conn = new ClientConnection();
    id->context = conn;
    conn->id = id;
    conn->qp = id->qp;
    conn->num_completions = 0;

    register_memory(conn, global_ctx->pd);
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
            throw std::runtime_error{fmt::format("Unknown event: {} ({})", event->event, rdma_event_str(event->event))};
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
    if (ec == nullptr) {
        throw std::runtime_error{fmt::format("Error creating event channel: {}", strerror(errno))};
    }


    rdma_cm_id *conn = nullptr;
    rdma_create_id(ec, &conn, nullptr, RDMA_PS_TCP); // TODO: Why TCP and not IB?
    if (conn == nullptr) {
        throw std::runtime_error{fmt::format("Error creating communication identifier: {}", strerror(errno))};
    }

    if (rdma_resolve_addr(conn, nullptr, addr->ai_addr, 500 /* ms timeout */) == -1) {
        throw std::runtime_error{fmt::format("Error resolving address: {}", strerror(errno))};

    }
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
