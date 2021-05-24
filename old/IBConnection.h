//
// Created by jonas on 06.04.21.
//

#ifndef INFINIBAND_IBCONNECTION_H
#define INFINIBAND_IBCONNECTION_H


#include "QueuePair.hpp"
#include "IBvMemoryRegion.hpp"
#include "IBvException.hpp"

#include <rdma/rdma_cma.h>
#include <thread>

class IBConnection {
    public:
        template<typename T>
        IBConnection(rdma_cm_id *id, T &&compCallback) :
                rdmaContext(id->verbs),
                rdmaProtectionDomain(rdmaContext),
                rdmaCompletionEventChannel(rdmaContext),
                rdmaCompletionQueue(rdmaContext, 10, rdmaCompletionEventChannel, 0),
                queuePair(rdma_qp(id, rdmaCompletionQueue.get(), rdmaProtectionDomain.get())),
                completionCallback(std::forward<T>(compCallback)),
                completionQueuePollerThread([this]() {

                    ibv_cq *cq;
                    ibv_wc wc{}; // Work Completion
                    void *ctx;

                    while (true) {
                        // Wait for channel
                        ibv_get_cq_event(rdmaCompletionEventChannel.get(), &cq, &ctx);
                        // Acknowledge completion event
                        ibv_ack_cq_events(cq, 1);
                        // Rearm completion queue
                        ibv_req_notify_cq(cq, 0);

                        // Poll all events from the queue
                        while (ibv_poll_cq(cq, 1, &wc)) {
                            if (completionCallback(wc)) {
                                return;
                            }
                        }
                    }
                }),
                send_mr(1000, rdmaProtectionDomain),
                recv_mr(1000, rdmaProtectionDomain) {
            throwIfError(ibv_req_notify_cq(rdmaCompletionQueue.get(), 0)); // TODO: what happens here? ->CQ class?
        }

        IBvContext rdmaContext;
        IBvProtectionDomain rdmaProtectionDomain;
        IBvCompletionEventChannel rdmaCompletionEventChannel;
        IBvCompletionQueue rdmaCompletionQueue;
        QueuePair queuePair;
        std::function<bool(const ibv_wc &)> completionCallback;
        std::jthread completionQueuePollerThread;
        IBvMemoryRegion<double> send_mr;
        IBvMemoryRegion<double> recv_mr;

    private:
        static ibv_qp *rdma_qp(rdma_cm_id *id, ibv_cq *cq, ibv_pd *pd);
};


#endif //INFINIBAND_IBCONNECTION_H
