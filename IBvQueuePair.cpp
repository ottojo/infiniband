//
// Created by jonas on 09.03.21.
//

#include "IBvQueuePair.hpp"
#include "IBvException.hpp"

IBvQueuePair::IBvQueuePair(IBvProtectionDomain &pd, IBvCompletionQueue &sendQC, IBvCompletionQueue &recvQC) {
    struct ibv_qp_init_attr initialQueuePairAttributes{
            .qp_context = pd.get()->context,
            .send_cq = sendQC.get(),
            .recv_cq = recvQC.get(),
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
    qp = ibv_create_qp(pd.get(), &initialQueuePairAttributes);
    if (qp == nullptr) {
        throw std::runtime_error("Creating queue pair failed");
    }
}

IBvQueuePair::~IBvQueuePair() {
    ibv_destroy_qp(qp);
}

struct ibv_qp *IBvQueuePair::get() {
    return qp;
}

enum ibv_qp_state IBvQueuePair::getState() {
    struct ibv_qp_attr attr{};
    struct ibv_qp_init_attr initAttr{};
    ibv_query_qp(qp, &attr, IBV_QP_STATE, &initAttr);
    return attr.qp_state;
}

void IBvQueuePair::initialize(uint8_t port) {
    Expects(getState() == IBV_QPS_RESET);

    // QP state: RESET -> INIT
    struct ibv_qp_attr queuePairAttributes{
            .qp_state = IBV_QPS_INIT, // Required for IBV_QP_STATE
            .qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ |
                               IBV_ACCESS_REMOTE_ATOMIC, // Required for IBV_QP_ACCESS_FLAGS
            .pkey_index = 0, // Required for IBV_QP_PKEY_INDEX
            .port_num = port, // Required for IBV_QP_PORT
    };
    throwIfError(ibv_modify_qp(qp,
                               &queuePairAttributes,
                               IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS));

    Ensures(getState() == IBV_QPS_INIT);
}
