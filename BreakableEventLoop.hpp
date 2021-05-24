/**
 * @file BreakableEventLoop.hpp
 * @author ottojo
 * @date 5/25/21
 * Description here TODO
 */

#ifndef INFINIBAND_BREAKABLEEVENTLOOP_HPP
#define INFINIBAND_BREAKABLEEVENTLOOP_HPP


#include <thread>
#include <functional>
#include <rdma/rdma_cma.h>

class BreakableEventLoop {
    public:
        using EventHandler = std::function<void(const rdma_cm_event &)>;

        BreakableEventLoop(rdma_event_channel *channel, EventHandler handler);

        ~BreakableEventLoop();

        void join();

    private:
        std::thread eventLoop;
        EventHandler eventHandler;
        int endEventLoopFD;
};


#endif //INFINIBAND_BREAKABLEEVENTLOOP_HPP
