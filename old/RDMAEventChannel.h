//
// Created by jonas on 06.04.21.
//

#ifndef INFINIBAND_RDMAEVENTCHANNEL_H
#define INFINIBAND_RDMAEVENTCHANNEL_H

#include <functional>
#include <fmt/format.h>
#include <rdma/rdma_cma.h>
#include <gsl/gsl>

class RDMAEventChannel {

    public:
        template<typename T>
        explicit RDMAEventChannel(T &&onEvent):
                ec(rdma_create_event_channel()),
                eventListener(std::forward<T>(onEvent)) {
        }

        ~RDMAEventChannel();

        RDMAEventChannel(const RDMAEventChannel &) = delete;

        RDMAEventChannel &operator=(const RDMAEventChannel &) = delete;

        void listen();

        rdma_event_channel *get();

    private:
        gsl::owner<rdma_event_channel *> ec = nullptr;

        // Event rdmaId should return true to stop listening for events
        std::function<bool(struct rdma_cm_event)> eventListener;
};


#endif //INFINIBAND_RDMAEVENTCHANNEL_H
