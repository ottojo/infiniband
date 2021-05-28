//
// Created by jonas on 22.05.21.
//

#ifndef INFINIBAND_RDMASERVER_HPP
#define INFINIBAND_RDMASERVER_HPP


#include <functional>
#include <rdma/rdma_cma.h>
#include <condition_variable>
#include "rdmaLib.hpp"
#include "BreakableEventLoop.hpp"
#include "BufferSet.hpp"

class RDMAServer : public BufferSet {
    public:
        RDMAServer(int port, std::size_t sendBufferSize, std::size_t recvBufferSize, std::function<void()> onConnect,
                   std::function<void(Buffer<char> &&b)> onReceive);

        ~RDMAServer();

        Buffer<char> getSendBuffer();

        void send(Buffer<char> &&b);

    private:
        std::function<void()> connectCallback;
        std::function<void(Buffer<char> &&b)> receiveCallback;

        std::unique_ptr<BreakableEventLoop> eventLoop;

        rdma_event_channel *ec = nullptr;

        gsl::owner<ibv_pd *> protectionDomain;

        std::unique_ptr<CompletionPoller> completionPoller = nullptr;

        gsl::owner<rdma_cm_id *> conn = nullptr;

        std::size_t sendBufferSize;
        std::size_t recvBufferSize;

        bool on_event(const rdma_cm_event &event);

        bool on_connection();

        bool on_disconnect();

        bool on_connect_request(gsl::owner<rdma_cm_id *> newConn);

        void on_completion(const ibv_wc &wc);

        void post_receives();
};

#endif //INFINIBAND_RDMASERVER_HPP
