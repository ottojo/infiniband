/**
 * @file CompletionPoller.cpp
 * @author ottojo
 * @date 5/29/21
 * Description here TODO
 */

#include <fmt/format.h>
#include <cassert>
#include "CompletionPoller.hpp"

gsl::owner<ibv_comp_channel *> createCC(ibv_context *ibvContext) {
    fmt::print("Creating completion channel\n");
    auto cc = ibv_create_comp_channel(ibvContext);
    // TODO: error handling
    return cc;
}

gsl::owner<ibv_cq *> createCQ(ibv_context *ibvContext, ibv_comp_channel *comp_channel) {
    fmt::print("Creating completion queue\n");
    auto cq = ibv_create_cq(ibvContext, 10, nullptr, comp_channel, 0);
    // Request notification for next event, even if not solicited
    ibv_req_notify_cq(cq, 0);
    return cq;
}

CompletionPoller::CompletionPoller(ibv_context *ibvContext, WCCallback workCompletionCallback) :
        comp_channel(createCC(ibvContext)),
        cq(createCQ(ibvContext, comp_channel)),
        breakable_poller_thread(comp_channel->fd, [this, callback = std::move(workCompletionCallback)]() {
            fmt::print("Completion poller\n");
            ibv_cq *wcQueue = nullptr;
            void *wcContext = nullptr;
            // Wait for next completion event in the channel
            ibv_get_cq_event(comp_channel, &wcQueue, &wcContext);

            // Expect all events in this channel to come from our queue/context
            assert(wcQueue == cq);

            // TODO: Verify handling of multiple events in queue (ack all, poll all? do we get 1 event per WC?)
            //  note:   Calling ibv_ack_cq_events() may be relatively expensive in the
            //       datapath, since it must take a mutex.  Therefore it may be better
            //       to amortize this cost by keeping a count of the number of events
            //       needing acknowledgement and acking several completion events in
            //       one call to ibv_ack_cq_events().

            // Acknowledge completion event
            ibv_ack_cq_events(wcQueue, 1);
            // Request a notification when the next Work Completion is added to the Completion Queue
            ibv_req_notify_cq(wcQueue, 0);
            // Empty the completion queue of WCs
            ibv_wc wc{};
            while (ibv_poll_cq(wcQueue, 1, &wc)) {
                callback(wc);
            }
        }) {
    (void) breakable_poller_thread;
}

CompletionPoller::~CompletionPoller() {
    ibv_destroy_cq(cq);
    ibv_destroy_comp_channel(comp_channel);
}
