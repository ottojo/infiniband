/**
 * @file BreakableEventLoop.cpp
 * @author ottojo
 * @date 5/25/21
 * Description here TODO
 */

#include <fmt/format.h>
#include <sys/eventfd.h>
#include "BreakableEventLoop.hpp"
#include "utils.hpp"

BreakableEventLoop::BreakableEventLoop(rdma_event_channel *ec, BreakableEventLoop::EventHandler handler) :
        eventHandler(std::move(handler)),
        endEventLoopFD(eventfd(0, 0)) {
    eventLoop = std::thread([this, ec]() {
        rdma_cm_event *event = nullptr;
        while (true) {
            auto availableFDs = waitForFDs(FileDescriptorSets{.read={ec->fd, endEventLoopFD}}, 1, 0);
            if (contains(availableFDs.read, ec->fd)) {
                fmt::print("Event loop: event channed fd can be read, polling event\n");
                if (rdma_get_cm_event(ec, &event) != 0) {
                    throw std::runtime_error{fmt::format("Error during rdma_get_cm_event: {}", strerror(errno))};
                }
                rdma_cm_event event_copy = *event;
                rdma_ack_cm_event(event);
                fmt::print("Received RDMA event of type \"{}\"\n", rdma_event_str(event->event));
                eventHandler(event_copy);
            } else if (contains(availableFDs.read, endEventLoopFD)) {
                fmt::print("Event loop: eventfd can be read, breaking\n");
                break;
            } else {
                throw std::runtime_error{"Error while waiting for events"};
            }
        }
        fmt::print("Event loop stopping.\n");
    });
}

void BreakableEventLoop::join() {
    fmt::print("Writing to eventfd to signal event loop to end\n");
    uint64_t v = 1;
    write(endEventLoopFD, &v, sizeof(v));

    fmt::print("Waiting for connection manager event loop\n");
    eventLoop.join();
}

BreakableEventLoop::~BreakableEventLoop() {
    join();
}
