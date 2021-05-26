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
#include "Buffer.hpp"
#include <map>
#include <queue>

class RDMAClient {
    public:
        using ReceiveCallback = std::function<void(const ibv_wc&, Buffer<char> &&)>;

        RDMAClient(std::string server, std::string port, std::size_t sendBufferSize, std::size_t recvBufferSize,
                   ReceiveCallback receiveCallback);

        ~RDMAClient();

        Buffer<char> getBuffer();

        void send(Buffer<char> &&b);

        void returnBuffer(Buffer<char> &&b);

    private:

        std::queue<Buffer<char>> sendBufferPool;
        std::map<char *, Buffer<char>> inFlightSendBuffers;
        std::queue<Buffer<char>> receiveBufferPool;
        std::map<char *, Buffer<char>> inFlightReceiveBuffers;

        const std::size_t recv_size;
        const std::size_t send_size;

        std::unique_ptr<Context> global_ctx = nullptr;
        gsl::owner<rdma_event_channel *> ec;

        ClientConnection *conn;

        ReceiveCallback receiveCallback;

        std::unique_ptr<BreakableEventLoop> eventLoop;

        std::promise<bool> connectedPromise;

        void on_completion(const ibv_wc &wc);


        void post_receives();

        bool on_connection();

        static bool on_disconnect(rdma_cm_id *id);

        bool on_address_resolved(rdma_cm_id *id);

        static bool on_route_resolved(rdma_cm_id *id);

        bool on_event(const rdma_cm_event &event);

};


#endif //INFINIBAND_RDMACLIENT_HPP
