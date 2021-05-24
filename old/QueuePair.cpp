//
// Created by jonas on 09.03.21.
//

#include "QueuePair.hpp"

struct ibv_qp *QueuePair::get() {
    return qp;
}

enum ibv_qp_state QueuePair::getState() {
    struct ibv_qp_attr attr{};
    struct ibv_qp_init_attr initAttr{};
    ibv_query_qp(qp, &attr, IBV_QP_STATE, &initAttr);
    return attr.qp_state;
}

QueuePair::QueuePair(ibv_qp *qp) : qp(qp) {}
