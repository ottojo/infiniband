/**
 * @file BreakableEventLoop.cpp
 * @author ottojo
 * @date 5/25/21
 * Description here TODO
 */

#include "BreakableEventLoop.hpp"
#include <fmt/format.h>

BreakableEventLoop::BreakableEventLoop(rdma_event_channel *ec, BreakableEventLoop::EventHandler handler) :
        BreakableFDWait(ec->fd, [ec, eventHandler = std::move(handler)]() {
            fmt::print("Event loop: event channed fd can be read, polling event\n");
            rdma_cm_event *event = nullptr;

            if (rdma_get_cm_event(ec, &event) != 0) {
                throw std::runtime_error{fmt::format("Error during rdma_get_cm_event: {}", strerror(errno))};
            }
            rdma_cm_event event_copy = *event;
            rdma_ack_cm_event(event);
            fmt::print("Received RDMA event of type \"{}\"\n", rdma_event_str(event->event));
            eventHandler(event_copy);
        }) {}
