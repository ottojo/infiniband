//
// Created by jonas on 06.04.21.
//

#include "RDMAEventChannel.h"
#include "RDMACmID.hpp"

void RDMAEventChannel::listen() {
    // Create the ID. We dont really need that one here, but each event contains a pointer to it...
    // TODO: find out what happens if we listen to events without an ID existing, and if we really dont need it to start
    //  listening
    RDMACmID id(*this);

    struct rdma_cm_event *event = nullptr;
    while (rdma_get_cm_event(ec, &event) == 0) {
        struct rdma_cm_event event_copy = *event;
        rdma_ack_cm_event(event);
        if (eventListener(event_copy)) {
            break;
        }
    }
}

RDMAEventChannel::~RDMAEventChannel() {
    rdma_destroy_event_channel(ec);
}

rdma_event_channel *RDMAEventChannel::get() {
    return ec;
}
