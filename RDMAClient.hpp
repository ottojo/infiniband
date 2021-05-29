/**
 * @file RDMAClient.hpp
 * @author ottojo
 * @date 5/24/21
 * Description here TODO
 */

#ifndef INFINIBAND_RDMACLIENT_HPP
#define INFINIBAND_RDMACLIENT_HPP

#include <string>
#include <functional>
#include <future>
#include "rdmaLib.hpp"
#include "BreakableEventLoop.hpp"
#include "BufferSet.hpp"
#include "CompletionPoller.hpp"
#include <map>
#include <queue>

class RDMAClient : public BufferSet {
    public:
        using ReceiveCallback = std::function<void(const ibv_wc &, Buffer<char> &&)>;

        RDMAClient(const std::string &server, const std::string &port, std::size_t sendBufferSize,
                   std::size_t recvBufferSize,
                   ReceiveCallback receiveCallback);

        ~RDMAClient();

        void send(Buffer<char> &&b);

        Buffer<char> getSendBuffer();


    private:


        gsl::owner<rdma_event_channel *> ec;

        gsl::owner<ibv_pd *> protectionDomain = nullptr;

        std::unique_ptr<CompletionPoller> completionPoller = nullptr;

        gsl::owner<rdma_cm_id *> conn = nullptr;

        ReceiveCallback receiveCallback;

        std::unique_ptr<BreakableFDWait> eventLoop;

        std::promise<bool> connectedPromise;

        void on_completion(const ibv_wc &wc);


        void post_receives();

        bool on_connection();

        bool on_disconnect();

        bool on_address_resolved();

        static bool on_route_resolved(rdma_cm_id *id);

        bool on_event(const rdma_cm_event &event);

};


#endif //INFINIBAND_RDMACLIENT_HPP
