//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_IBVCOMPLETIONQUEUE_HPP
#define INFINIBAND_IBVCOMPLETIONQUEUE_HPP

#include "IBvContext.hpp"
#include "IBvCompletionEventChannel.hpp"

class IBvCompletionQueue {
    public:
        IBvCompletionQueue(IBvContext &context, int minCapacity, void *queueContext, IBvCompletionEventChannel &channel,
                           int comp_vector);

        IBvCompletionQueue(IBvContext &context, int minCapacity, IBvCompletionEventChannel &channel, int comp_vector);

        // TODO: Ctor without channel (nullptr)

        IBvCompletionQueue(const IBvCompletionQueue &) = delete;

        IBvCompletionQueue &operator=(const IBvCompletionQueue &) = delete;

        struct ibv_cq *get();

    private:
        gsl::owner<struct ibv_cq *> cq;
};


#endif //INFINIBAND_IBVCOMPLETIONQUEUE_HPP
