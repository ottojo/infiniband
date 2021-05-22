//
// Created by jonas on 22.05.21.
//

#ifndef INFINIBAND_RDMALIB_HPP
#define INFINIBAND_RDMALIB_HPP

#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <thread>
#include <vector>

constexpr auto WIDTH = 1920;
constexpr auto HEIGHT = 1080;
constexpr auto BUFFER_SIZE = WIDTH * HEIGHT;

struct ClientConnection {
    rdma_cm_id *id;
    ibv_qp *qp;
    ibv_mr *send_mr;
    ibv_mr *recv_mr;
    char *recv_region;
    char *send_region;
    int num_completions;
};


struct ServerConnection {
    ibv_qp *qp;
    ibv_mr *send_mr;
    ibv_mr *recv_mr;
    std::vector<char> recv_region;
    std::vector<char> send_region;
};


struct Context {
    ibv_context *ctx;
    ibv_pd *pd;
    ibv_cq *cq;
    ibv_comp_channel *comp_channel;

    std::thread cq_poller_thread;
};


ibv_qp_init_attr build_qp_attr(Context *s_ctx);

#endif //INFINIBAND_RDMALIB_HPP
