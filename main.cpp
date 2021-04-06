#include <iostream>
#include <infiniband/verbs.h>
#include "IBvDeviceList.hpp"
#include "IBvContext.hpp"
#include <fmt/format.h>
#include <gsl/gsl>
#include "libibverbs_format.hpp"
#include "IBvProtectionDomain.hpp"
#include "IBvCompletionEventChannel.hpp"
#include "IBvCompletionQueue.hpp"
#include "IBvQueuePair.hpp"
#include "IBvMemoryRegion.hpp"
#include "IBvException.hpp"
#include "RDMAEventChannel.h"
#include "IBConnection.h"

#include <rdma/rdma_cma.h>
#include <thread>

enum class Role {
        Client,
        Server
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fmt::print("Not enough arguments. Specify server or client\n");
        return 1;
    }

    auto role = Role::Server;
    if (std::string{argv[1]} == "client") {
        role = Role::Client;
    }

    auto deviceList = IBvDeviceList();
    fmt::print("Found {} devices\n", deviceList.size());
    if (deviceList.empty()) {
        return 0;
    }

    for (const auto d:deviceList) {
        fmt::print(FMT_STRING("Name: {}, dev name: {}, device name: {}, GUID: {}, node type: {}\n"),
                   d->name,
                   d->dev_name,
                   ibv_get_device_name(d),
                   ibv_get_device_guid(d),
                   ibv_node_type_str(d->node_type));
    }

    auto context = IBvContext(deviceList[0]);

    auto attributes = context.queryAttributes();
    for (int p = 1; p < attributes.phys_port_cnt + 1; p++) {
        // auto portAttributes = context.queryPort(p);
        ibv_gid guid{};
        auto err = ibv_query_gid(context.get(), p, 0, &guid);
        throwIfError(err);

        fmt::print("Port {} has GUID: (subnet-prefix: {}, interface-id: {})\n",
                   p,
                   guid.global.subnet_prefix,
                   guid.global.interface_id);
    }
    Expects(attributes.phys_port_cnt > 0);
    uint8_t portNumber = 1;

    // Allocate protection domain
    auto protectionDomain = IBvProtectionDomain(context);
    fmt::print(FMT_STRING("Allocated protaction domain {}\n"), protectionDomain.get()->handle);

    // Register memory region MR
    auto memoryRegion = IBvMemoryRegion<double>(1000, protectionDomain);
    fmt::print(FMT_STRING("Registered memory region {} at address {} with size {} and LKey {}, RKey {}\n"),
               memoryRegion.get()->handle, memoryRegion.get()->addr, memoryRegion.get()->length,
               memoryRegion.get()->lkey, memoryRegion.get()->rkey);

    // Create send and receive completion queue
    auto completionEventChannel = IBvCompletionEventChannel(context);

    auto sendCompletionQueue = IBvCompletionQueue(context, 100, completionEventChannel, 0);

    auto recvCompletionQueue = IBvCompletionQueue(context, 100, completionEventChannel, 0);

    // Create queue pair QP
    auto queuePair = IBvQueuePair(protectionDomain, sendCompletionQueue, recvCompletionQueue);
    fmt::print(FMT_STRING("Created queue pair {}, state {}\n"), queuePair.get()->handle, queuePair.getState());


    // Initialize queue pair (queue state should be INIT), for our Reliable Connected QP this also means establishing
    //  the connection
    queuePair.initialize(portNumber);

    fmt::print(FMT_STRING("Queue pair is in state {}\n"), queuePair.getState());

    // Exchange info (out of band):
    //  Local ID LID (assigned by subnet manager)
    //  Queue Pair Number QPN (assigned by Host Channel Adapter HCA)
    //  Packet Sequence Number PSN
    //  Remote Key R_Key which allows peer to access local MR
    //  Memory address VADDR for peer

    //auto rdmacmEventChannel = IBvCompletionEventChannel


    // TODO error handling

    if (role == Role::Server) {


        std::optional<IBConnection> connection;


        RDMAEventChannel ec(
                [&connection]
                        (const struct rdma_cm_event &event) {
                    fmt::print("Server received event with status {}\n", event.status);

                    if (event.event == RDMA_CM_EVENT_CONNECT_REQUEST) {
                        fmt::print("connect request event\n");

                        if (not connection.has_value()) {
                            connection.emplace(event.id->verbs);
                        } else if (connection->rdmaContext.get() != event.id->verbs) {
                            throw std::runtime_error{"Event from different context?"};
                        }

                        // TODO: CQ Poller thread



                        // TODO: QP handling with librdmacm
                        //  http://www.hpcadvisorycouncil.com/pdf/building-an-rdma-capable-application-with-ib-verbs.pdf
                        struct ibv_qp_init_attr qp_attr{};
                        qp_attr.send_cq = connection->rdmaCompletionQueue.get();
                        qp_attr.recv_cq = connection->rdmaCompletionQueue.get();
                        qp_attr.qp_type = IBV_QPT_RC;
                        qp_attr.cap.max_send_wr = 10;
                        qp_attr.cap.max_recv_wr = 10;
                        qp_attr.cap.max_send_sge = 1;
                        qp_attr.cap.max_recv_sge = 1;

                        rdma_create_qp(event.id, connection->rdmaProtectionDomain.get(), &qp_attr);

                        event.id->context = &connection;
                        connection->queuePair = event.id->qp;

                        // TODO: Register memory
                        // TODO: postr_receives
                        // TODO: rdma_accept

                    } else if (event.event == RDMA_CM_EVENT_ESTABLISHED) {
                        fmt::print("established event\n");

                    } else if (event.event == RDMA_CM_EVENT_DISCONNECTED) {
                        fmt::print("disconnected event\n");
                        // Stop listening
                        return true;
                    }

                    return false;
                });

        ec.listen();

        //queuePair.receive();
    }

    if (role == Role::Client) {

        //queuePair.send();

    }

    // TODO: RDMA write
}
