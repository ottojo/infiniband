//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_IBVQUEUEPAIR_HPP
#define INFINIBAND_IBVQUEUEPAIR_HPP


#include <gsl/gsl>
#include "IBvProtectionDomain.hpp"
#include "IBvCompletionQueue.hpp"

/**
 * Queue pair, currently supports only "Reliable Connected" type
 */
class IBvQueuePair {
    public:
        IBvQueuePair(IBvProtectionDomain &pd, IBvCompletionQueue &sendQC, IBvCompletionQueue &recvQC);

        ~IBvQueuePair();

        IBvQueuePair(const IBvQueuePair &) = delete;

        IBvQueuePair &operator=(const IBvQueuePair &) = delete;

        struct ibv_qp *get();

        enum ibv_qp_state getState();

        /**
         * Changes state from RESET to INIT
         */
        void initialize(uint8_t port);

    private:
        gsl::owner<struct ibv_qp *> qp;
};


#endif //INFINIBAND_IBVQUEUEPAIR_HPP
