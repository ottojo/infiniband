//
// Created by jonas on 22.05.21.
//

#include "rdmaLib.hpp"

ibv_qp_init_attr build_qp_attr(Context *s_ctx) {
    ibv_qp_init_attr qp_attr{};
    qp_attr.send_cq = s_ctx->cq;
    qp_attr.recv_cq = s_ctx->cq;
    qp_attr.qp_type = IBV_QPT_RC;

    qp_attr.cap.max_send_wr = 10;
    qp_attr.cap.max_recv_wr = 10;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;
    return qp_attr;
}
