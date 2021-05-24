//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_QUEUEPAIR_HPP
#define INFINIBAND_QUEUEPAIR_HPP


#include <gsl/gsl>
#include "IBvProtectionDomain.hpp"
#include "IBvCompletionQueue.hpp"

/**
 * Queue pair (non-owning, ibv_qp* usually handled by librdmacm)
 */
class QueuePair {
    public:
        explicit QueuePair(ibv_qp *qp);

        ibv_qp *get();

        enum ibv_qp_state getState();

    private:
        ibv_qp *qp;
};


#endif //INFINIBAND_QUEUEPAIR_HPP
