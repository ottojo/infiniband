//
// Created by jonas on 09.03.21.
//

#include "IBvCompletionQueue.hpp"

IBvCompletionQueue::IBvCompletionQueue(IBvContext &context, int minCapacity, void *queueContext,
                                       IBvCompletionEventChannel &channel, int comp_vector) :
        cq{ibv_create_cq(context.get(), minCapacity, queueContext, channel.get(), comp_vector)} {
    if (cq == nullptr) {
        throw std::runtime_error{"Creating completion queue failed"};
    }
}

IBvCompletionQueue::IBvCompletionQueue(IBvContext &context, int minCapacity, IBvCompletionEventChannel &channel,
                                       int comp_vector) :
        IBvCompletionQueue(context, minCapacity, nullptr, channel, comp_vector) {}

struct ibv_cq *IBvCompletionQueue::get() {
    return cq;
}
