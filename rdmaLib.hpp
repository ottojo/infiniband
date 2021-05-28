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

/*
struct ServerConnection {
    ibv_qp *qp;
    ibv_mr *send_mr;
    ibv_mr *recv_mr;
    std::vector<char> recv_region;
    std::vector<char> send_region;
};
*/

class CompletionPoller {
    public:
        using WCCallback = std::function<void(const ibv_wc &wc)>;

        CompletionPoller(ibv_context *ibvContext, WCCallback workCompletionCallback);

        ~CompletionPoller();

        gsl::owner<ibv_cq *> cq;
        gsl::owner<ibv_comp_channel *> comp_channel;
    private:
        std::thread cq_poller_thread; // TODO: properly join this thread
        WCCallback workCompletionCallback;
};


ibv_qp_init_attr build_qp_attr(ibv_cq *cq);

#endif //INFINIBAND_RDMALIB_HPP
