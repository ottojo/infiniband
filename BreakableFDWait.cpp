/**
 * @file BreakableThing.cpp
 * @author ottojo
 * @date 5/29/21
 * Description here TODO
 */

#include "BreakableFDWait.hpp"
#include "utils.hpp"
#include <fmt/format.h>
#include <sys/eventfd.h>
#include <unistd.h>

BreakableFDWait::BreakableFDWait(int fd, BreakableFDWait::EventHandler handler) :
        eventHandler(std::move(handler)),
        endEventLoopFD(eventfd(0, 0)) {
    eventLoop = std::thread([this, fd]() {
        while (true) {
            auto availableFDs = waitForFDs(FileDescriptorSets{.read={fd, endEventLoopFD}}, 1, 0);
            if (contains(availableFDs.read, fd)) {
                eventHandler();
            } else if (contains(availableFDs.read, endEventLoopFD)) {
                fmt::print("Event loop: eventfd can be read, breaking\n");
                break;
            } else {
                throw std::runtime_error{"Error while waiting for events"};
            }
        }
        fmt::print("Breakable thing stopping.\n");
    });
}

BreakableFDWait::~BreakableFDWait() {
    join();
}

void BreakableFDWait::join() {
    fmt::print("Writing to eventfd to signal event loop to end\n");
    uint64_t v = 1;
    write(endEventLoopFD, &v, sizeof(v));

    fmt::print("Waiting for event loop\n");
    eventLoop.join();
}
