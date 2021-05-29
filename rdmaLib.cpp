//
// Created by jonas on 22.05.21.
//

#include <fmt/format.h>
#include <cassert>
#include "rdmaLib.hpp"

ibv_qp_init_attr build_qp_attr(ibv_cq *cq) {
    ibv_qp_init_attr qp_attr{};
    qp_attr.send_cq = cq;
    qp_attr.recv_cq = cq;
    qp_attr.qp_type = IBV_QPT_RC;

    qp_attr.cap.max_send_wr = 10;
    qp_attr.cap.max_recv_wr = 10;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;
    return qp_attr;
}
