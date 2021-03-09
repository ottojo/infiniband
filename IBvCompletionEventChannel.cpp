//
// Created by jonas on 09.03.21.
//

#include "IBvCompletionEventChannel.hpp"

IBvCompletionEventChannel::IBvCompletionEventChannel(IBvContext &context) :
        channel{ibv_create_comp_channel(context.get())} {
    if (channel == nullptr) {
        throw std::runtime_error{"Creating completion event channel failed"};
    }
}

IBvCompletionEventChannel::~IBvCompletionEventChannel() {
    ibv_destroy_comp_channel(channel);
}

struct ibv_comp_channel *IBvCompletionEventChannel::get() {
    return channel;
}
