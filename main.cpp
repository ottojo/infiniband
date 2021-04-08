#include <iostream>
#include <infiniband/verbs.h>
#include "IBvDeviceList.hpp"
#include <fmt/format.h>
#include "IBvException.hpp"
#include "RDMAEventChannel.h"
#include "IBConnection.h"

#include <rdma/rdma_cma.h>

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

    // TODO error handling

    if (role == Role::Server) {

        // This contains all the state that forms a "connection", such as completion queues
        std::optional<IBConnection> connection;

        // Create an event channel. Everything interesting happens when some event occurs.
        // The event channel binds to a port (TCP/IP? Or also IB? Both?) to exchange connection information
        RDMAEventChannel ec(
                [&connection]
                        (const struct rdma_cm_event &event) {
                    fmt::print("Server received event with status {}\n", event.status);

                    // The passive (server) side is only interested in connect request, connect, and disconnect events.

                    if (event.event == RDMA_CM_EVENT_CONNECT_REQUEST) {
                        fmt::print("connect request event\n");

                        // We build the context etc once we receive the first connection request, since that will be
                        // bound to a specific device. The connection request already has a valid ibverbs context at
                        // event.id->verbs
                        if (not connection.has_value()) {
                            connection.emplace(event.id, [](const ibv_wc &wc) {
                                // TODO on completion
                                fmt::print("Received work completion: {}\n", wc.wr_id);
                                return false;
                            });
                        } else if (connection->rdmaContext.get() != event.id->verbs) {
                            throw std::runtime_error{"Event from different context?"};
                        }

                        //  http://www.hpcadvisorycouncil.com/pdf/building-an-rdma-capable-application-with-ib-verbs.pdf

                        // Store that connection with the ID
                        event.id->context = &connection;

                        // Pre-post receives (receive WRs): The underlying hardware wont buffer incoming messages: If a
                        // message comes in without a receive request posted to the queue, the incoming message is
                        // rejected and the peer receives a receiver-not-ready (RNR) error.

                        // A Scatter/Gather Element SGE is a pointer to a memory region which the Host Channel Adapter can read from or write into
                        ibv_sge sge{};
                        sge.addr = reinterpret_cast<uint64_t>(connection->recv_mr.data().get());
                        sge.length = connection->recv_mr.size() * connection->recv_mr.elementSize();
                        sge.lkey = connection->recv_mr.lkey();

                        ibv_recv_wr wr{};
                        wr.wr_id = reinterpret_cast<uint64_t>(&connection); // User defined. Tutorial stores &connection, lets see if that is important later
                        wr.next = nullptr; // This is technically a linked list of WRs
                        wr.sg_list = &sge;
                        wr.num_sge = 1;

                        ibv_recv_wr *bad_wr = nullptr; // Return first failed WR through this
                        ibv_post_recv(connection->queuePair.get(), &wr, &bad_wr);

                        rdma_conn_param connParam{};
                        rdma_accept(event.id, &connParam);

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
