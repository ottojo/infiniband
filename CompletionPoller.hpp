/**
 * @file CompletionPoller.hpp
 * @author ottojo
 * @date 5/29/21
 * Description here TODO
 */

#ifndef INFINIBAND_COMPLETIONPOLLER_HPP
#define INFINIBAND_COMPLETIONPOLLER_HPP

#include <functional>
#include <thread>
#include <gsl/gsl>
#include <infiniband/verbs.h>
#include "BreakableFDWait.hpp"

class CompletionPoller {
    public:
        using WCCallback = std::function<void(const ibv_wc &wc)>;

        CompletionPoller(ibv_context *ibvContext, WCCallback workCompletionCallback);

        ~CompletionPoller();

        gsl::owner<ibv_comp_channel *> comp_channel;
        gsl::owner<ibv_cq *> cq;
    private:

        BreakableFDWait breakable_poller_thread;
};


#endif //INFINIBAND_COMPLETIONPOLLER_HPP
