//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_IBVCOMPLETIONEVENTCHANNEL_HPP
#define INFINIBAND_IBVCOMPLETIONEVENTCHANNEL_HPP

#include <infiniband/verbs.h>
#include <gsl/gsl>
#include "IBvContext.hpp"

class IBvCompletionEventChannel {
    public:
        explicit IBvCompletionEventChannel(IBvContext &context);

        ~IBvCompletionEventChannel();

        IBvCompletionEventChannel(const IBvCompletionEventChannel &) = delete;

        IBvCompletionEventChannel &operator=(const IBvCompletionEventChannel &) = delete;

        struct ibv_comp_channel *get();

    private:
        gsl::owner<struct ibv_comp_channel *> channel;
};


#endif //INFINIBAND_IBVCOMPLETIONEVENTCHANNEL_HPP
