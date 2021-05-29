//
// Created by jonas on 22.05.21.
//

#ifndef INFINIBAND_RDMALIB_HPP
#define INFINIBAND_RDMALIB_HPP

#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <thread>
#include <vector>
#include <functional>

#include <gsl/gsl>

constexpr auto WIDTH = 1920;
constexpr auto HEIGHT = 1080;
constexpr auto BUFFER_SIZE = WIDTH * HEIGHT;

ibv_qp_init_attr build_qp_attr(ibv_cq *cq);

#endif //INFINIBAND_RDMALIB_HPP
