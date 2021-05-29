/**
 * @file BreakableEventLoop.hpp
 * @author ottojo
 * @date 5/25/21
 * Description here TODO
 */

#ifndef INFINIBAND_BREAKABLEFDWAIT_HPP
#define INFINIBAND_BREAKABLEFDWAIT_HPP


#include <thread>
#include <functional>


class BreakableFDWait {
    public:
        using EventHandler = std::function<void()>;

        BreakableFDWait(int fd, EventHandler handler);

        ~BreakableFDWait();

    private:

        std::thread eventLoop;
        EventHandler eventHandler;
        int endEventLoopFD;

        void join();
};

#endif //INFINIBAND_BREAKABLEFDWAIT_HPP
