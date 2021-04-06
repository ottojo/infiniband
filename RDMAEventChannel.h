//
// Created by jonas on 06.04.21.
//

#ifndef INFINIBAND_RDMAEVENTCHANNEL_H
#define INFINIBAND_RDMAEVENTCHANNEL_H

#include <functional>
#include <fmt/format.h>
#include <rdma/rdma_cma.h>

class RDMAEventChannel {

public:
    template<typename T>
    explicit RDMAEventChannel(T &&onEvent):
            eventListener(std::forward<T>(onEvent)) {
        ec = rdma_create_event_channel();
        rdma_create_id(ec, &listener, nullptr, RDMA_PS_TCP);
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = 0; // Choose port for us
        rdma_bind_addr(listener, (struct sockaddr *) &addr);
        rdma_listen(listener, 10);
        auto port = ntohs(rdma_get_src_port(listener));
        fmt::print("Listening on port {}\n", port);
    }

    ~RDMAEventChannel();

    RDMAEventChannel(const RDMAEventChannel &) = delete;

    RDMAEventChannel &operator=(const RDMAEventChannel &) = delete;

    void listen();

private:
    struct rdma_cm_id *listener = nullptr;

    struct rdma_event_channel *ec = nullptr;

    // Event listener should return true to stop listening for events
    std::function<bool(struct rdma_cm_event)> eventListener;
};


#endif //INFINIBAND_RDMAEVENTCHANNEL_H
