//
// Created by jonas on 06.04.21.
//

#include "RDMAEventChannel.h"

void RDMAEventChannel::listen() {
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
    rdma_destroy_id(listener);
    rdma_destroy_event_channel(ec);
}
