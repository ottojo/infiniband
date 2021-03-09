#include <iostream>
#include <infiniband/verbs.h>
#include <infiniband/ib.h>
#include "IBvDeviceList.hpp"
#include "IBvContext.hpp"
#include <fmt/format.h>
#include <vector>
#include <gsl/gsl>
#include "libibverbs_format.hpp"
#include "IBvProtectionDomain.hpp"

void errCheck(int err) {
    if (err != 0) {
        fmt::print("Error {}: {}\n", err, strerror(err));
        throw std::runtime_error{strerror(err)};
    }
}

int main() {
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
        errCheck(err);

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
    std::vector<double> memoryRegionVector(1000, 0.0);
    auto memoryRegion = ibv_reg_mr(protectionDomain.get(), static_cast<void *>(memoryRegionVector.data()),
                                   memoryRegionVector.size() * sizeof(decltype(memoryRegionVector)::value_type),
                                   IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_WRITE);
    if (memoryRegion == nullptr) {
        throw std::runtime_error("Registering memory region failed");
    }
    auto mr_final = gsl::finally([&memoryRegion]() {
        errCheck(ibv_dereg_mr(memoryRegion));
    });

    fmt::print(FMT_STRING("Registered memory region {} at address {} with size {} and LKey {}, RKey {}\n"),
               memoryRegion->handle, memoryRegion->addr, memoryRegion->length, memoryRegion->lkey, memoryRegion->rkey);

    // Create send and receive completion queue
    auto completionEventChannel = ibv_create_comp_channel(context.get());
    auto cec_finally = gsl::finally([&completionEventChannel]() {
        errCheck(ibv_destroy_comp_channel(completionEventChannel));
    });

    auto sendCompletionQueue = ibv_create_cq(context.get(), 100, nullptr, completionEventChannel, 0);
    auto sendQC_finally = gsl::finally([&sendCompletionQueue]() {
        errCheck(ibv_destroy_cq(sendCompletionQueue));
    });

    auto recvCompletionQueue = ibv_create_cq(context.get(), 100, nullptr, completionEventChannel, 0);
    auto recvCQ_finally = gsl::finally([&recvCompletionQueue]() {
        errCheck(ibv_destroy_cq(recvCompletionQueue));
    });

    // Create queue pair QP
    struct ibv_qp_init_attr initialQueuePairAttributes{
            .qp_context = context.get(),
            .send_cq = sendCompletionQueue,
            .recv_cq = recvCompletionQueue,
            .srq = nullptr,
            .cap = {
                    .max_send_wr=2,
                    .max_recv_wr=2,
                    .max_send_sge=1, // TODO: learn about scatter/gather elements
                    .max_recv_sge=1,
                    .max_inline_data=0
            },
            .qp_type = IBV_QPT_RC, // Reliable Connected
            .sq_sig_all = 0,
    };
    auto queuePair = ibv_create_qp(protectionDomain.get(), &initialQueuePairAttributes);
    if (queuePair == nullptr) {
        throw std::runtime_error("Creating queue pair failed");
    }
    auto qp_finally = gsl::finally([&queuePair]() {
        ibv_destroy_qp(queuePair);
    });
    fmt::print(FMT_STRING("Created queue pair {}, state {}\n"), queuePair->handle, queuePair->state);



    // TODO: Initialize queue pair (queue state should be INIT), for our Reliable Connected QP this also means
    //  establishing the connection

    // QP state: RESET -> INIT
    struct ibv_qp_attr queuePairAttributes{
            .qp_state = IBV_QPS_INIT, // Required for IBV_QP_STATE
            .qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ |
                               IBV_ACCESS_REMOTE_ATOMIC, // Required for IBV_QP_ACCESS_FLAGS
            .pkey_index = 0, // Required for IBV_QP_PKEY_INDEX
            .port_num = portNumber, // Required for IBV_QP_PORT
    };
    errCheck(ibv_modify_qp(queuePair,
                           &queuePairAttributes,
                           IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS));

    {
        struct ibv_qp_attr attr{};
        struct ibv_qp_init_attr initAttr{};
        ibv_query_qp(queuePair, &attr, IBV_QP_STATE, &initAttr);
        fmt::print(FMT_STRING("Queue pair is in state {}\n"), attr.qp_state);
        Expects(attr.qp_state == IBV_QPS_INIT);
    }

    // TODO: Exchange info (out of band):
    //  Local ID LID (assigned by subnet manager)
    //  Queue Pair Number QPN (assigned by Host Channel Adapter HCA)
    //  Packet Sequence Number PSN
    //  Remote Key R_Key which allows peer to access local MR
    //  Memory address VADDR for peer

    // TODO: Change QP status to Ready to Receive RTR
    // TODO, Server: Change QP status to Ready To Send RTS

    // TODO: RDMA write




    //ibv_post_send();
}
