//
// Created by jonas on 06.04.21.
//

#include "IBConnection.h"

IBConnection::IBConnection(struct ibv_context *ctx) :
        rdmaContext(ctx),
        rdmaProtectionDomain(rdmaContext),
        rdmaCompletionEventChannel(rdmaContext),
        rdmaCompletionQueue(rdmaContext, 10, rdmaCompletionEventChannel, 0) {
    ibv_req_notify_cq(rdmaCompletionQueue.get(), 0); // TODO: what happens here? ->CQ class?
}
