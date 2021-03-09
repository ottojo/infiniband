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
#include "httpClient.hpp"
#include "httpServer.hpp"

enum class Role {
        Sender,
        Receiver
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fmt::print("Not enough arguments. Specify sender or receiver\n");
        return 1;
    }

    auto role = Role::Receiver;
    if (std::string{argv[1]} == "sender") {
        role = Role::Sender;
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

    auto portAttr = context.queryPort(portNumber);
    ConnectionInfo myInfo{
            .localLID = portAttr.lid,
            .queuePairNumber = queuePair.get()->qp_num,
            .packetSequenceNumber = 0,
            .remoteKey = memoryRegion.get()->rkey,
            .vaddr = 7};

    if (role == Role::Receiver) {
        // Receiver: Connect to server, send info, receive info
        auto senderInfo = exchangeInfo("localhost", "1337", "/connectionInfo", myInfo);
        nlohmann::json s = senderInfo;
        fmt::print("Got remote info: {}\n", s.dump());
        // TODO, Receiver: Change QP status to Ready to Receive RTR
    }

    if (role == Role::Sender) {
        // Sender: Open server, receive info, reply info
        auto receiverInfo = serveInfo(1337, myInfo);
        nlohmann::json s = receiverInfo;
        fmt::print("Got remote info: {}\n", s.dump());
        // TODO, Sender: Change QP status to Ready To Send RTS
    }

    // TODO: RDMA write
}
