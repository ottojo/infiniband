//
// Created by jonas on 22.05.21.
//

#ifndef INFINIBAND_RDMASERVER_HPP
#define INFINIBAND_RDMASERVER_HPP


#include <functional>
#include <rdma/rdma_cma.h>
#include <condition_variable>
#include "rdmaLib.hpp"


class RDMAServer {
    public:
        RDMAServer(int port, std::size_t sendBufferSize, std::size_t recvBufferSize, std::function<void()> onConnect,
                   std::function<void(RDMAServer &server, ServerConnection &connection)> onReceive);

        ~RDMAServer();

        void send(ServerConnection &connection);

    private:
        std::function<void()> connectCallback;
        std::function<void(RDMAServer &server, ServerConnection &connection)> receiveCallback;

        std::thread eventLoop;
        bool eventLoopEnd = false;

        rdma_event_channel *ec = nullptr;
        rdma_cm_id *listener = nullptr;


        std::unique_ptr<Context> s_ctx = nullptr;

        std::size_t sendBufferSize;
        std::size_t recvBufferSize;

        bool on_event(rdma_cm_event *event);

        bool on_connection(void *context);

        bool on_disconnect(rdma_cm_id *id);

        bool on_connect_request(rdma_cm_id *id);

        void on_completion(const ibv_wc &wc);


        void build_context(ibv_context *verbsContext);


        void register_memory(ServerConnection *conn);


        void post_receives(ServerConnection *conn);

        ibv_qp_init_attr build_qp_attr();


};

#endif //INFINIBAND_RDMASERVER_HPP
