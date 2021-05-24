//
// Created by jonas on 22.05.21.
//

#include <fmt/format.h>
#include <cassert>
#include "rdmaLib.hpp"

ibv_qp_init_attr build_qp_attr(Context *s_ctx) {
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

Context::Context(gsl::owner<ibv_context *> ibvContext, WCCallback workCompletionCallback) :
        ctx(ibvContext),
        workCompletionCallback(std::move(workCompletionCallback)) {

    fmt::print("Allocating protection domain\n");
    pd = ibv_alloc_pd(ctx);
    fmt::print("Creating completion channel\n");
    comp_channel = ibv_create_comp_channel(ctx);
    fmt::print("Creating completion queue\n");
    cq = ibv_create_cq(ctx, 10, nullptr, comp_channel, 0);
    // Request notification for next event, even if not solicited
    ibv_req_notify_cq(cq, 0);

    // TODO: join/cancel this? May need to poll instead of get_cq_event
    cq_poller_thread = std::thread([this]() {
        while (true) {
            ibv_cq *wcQueue = nullptr;
            void *wcContext = nullptr;
            // Wait for next completion event in the channel
            ibv_get_cq_event(comp_channel, &wcQueue, &wcContext);

            // Expect all events in this channel to come from our queue/context
            assert(wcQueue == cq);
            assert(wcContext == this);

            // TODO: Verify handling of multiple events in queue (ack all, poll all? do we get 1 event per WC?)
            // Acknowledge completion event
            ibv_ack_cq_events(wcQueue, 1);
            // Request a notification when the next Work Completion is added to the Completion Queue
            ibv_req_notify_cq(wcQueue, 0);
            // Empty the completion queue of WCs
            ibv_wc wc{};
            while (ibv_poll_cq(wcQueue, 1, &wc)) {
                this->workCompletionCallback(wc);
            }
        }
    });
}

Context::~Context() {
    // cq_poller_thread.join();
    if (cq_poller_thread.joinable()) {
        fmt::print("TODO: join completion queue poller thread\n");
    }

    ibv_destroy_cq(cq);
    ibv_destroy_comp_channel(comp_channel);
    ibv_dealloc_pd(pd);

}
