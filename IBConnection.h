//
// Created by jonas on 06.04.21.
//

#ifndef INFINIBAND_IBCONNECTION_H
#define INFINIBAND_IBCONNECTION_H


#include "IBvQueuePair.hpp"
#include "IBvMemoryRegion.hpp"

class IBConnection {
    public:
        IBConnection(struct ibv_context *ctx);

        IBvContext rdmaContext;
        IBvProtectionDomain rdmaProtectionDomain;
        IBvCompletionEventChannel rdmaCompletionEventChannel;
        IBvCompletionQueue rdmaCompletionQueue;
        ibv_qp *queuePair; // TODO QP handling
};


#endif //INFINIBAND_IBCONNECTION_H
