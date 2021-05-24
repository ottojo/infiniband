//
// Created by jonas on 06.04.21.
//

#include "IBConnection.h"

ibv_qp *IBConnection::rdma_qp(rdma_cm_id *id, ibv_cq *cq, ibv_pd *pd) {
    ibv_qp_init_attr qp_attr{};
    qp_attr.send_cq = cq;
    qp_attr.recv_cq = cq;
    qp_attr.qp_type = IBV_QPT_RC;
    qp_attr.cap.max_send_wr = 10;
    qp_attr.cap.max_recv_wr = 10;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;

    throwIfErrorErrno(rdma_create_qp(id, pd, &qp_attr));
    return id->qp;
}
