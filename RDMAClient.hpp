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
#include "rdmaLib.hpp"

class RDMAClient {
    public:
        RDMAClient(std::string server, std::string port, std::size_t sendBufferSize,
                   std::function<void(ClientConnection *)> receiveCallback);

        ~RDMAClient();

    private:
        std::unique_ptr<Context> global_ctx = nullptr;
        rdma_event_channel *ec;
        std::function<void(ClientConnection *)> receiveCallback;

        void on_completion(const ibv_wc &wc);

        void register_memory(ClientConnection *conn, ibv_pd *pd);

        void post_receives(ClientConnection *conn);

        bool on_connection(void *context);

        bool on_disconnect(rdma_cm_id *id);

        bool on_address_resolved(rdma_cm_id *id);

        bool on_route_resolved(rdma_cm_id *id);

/**
 * @return true to end event loop
 */
        bool on_event(rdma_cm_event *event);

};


#endif //INFINIBAND_RDMACLIENT_HPP
