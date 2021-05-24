//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_LIBIBVERBS_FORMAT_HPP
#define INFINIBAND_LIBIBVERBS_FORMAT_HPP

#include <fmt/format.h>
#include <infiniband/verbs.h>

template<>
struct fmt::formatter<enum ibv_qp_state> : formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template<typename FormatContext>
    auto format(enum ibv_qp_state c, FormatContext &ctx) {
        string_view name = "unknown";
        switch (c) {
            case IBV_QPS_RESET:
                name = "RESET";
                break;
            case IBV_QPS_INIT:
                name = "INIT";
                break;
            case IBV_QPS_RTR:
                name = "RTR";
                break;
            case IBV_QPS_RTS:
                name = "RTS";
                break;
            case IBV_QPS_SQD:
                name = "SQD";
                break;
            case IBV_QPS_SQE:
                name = "SQE";
                break;
            case IBV_QPS_ERR:
                name = "ERR";
                break;
            case IBV_QPS_UNKNOWN:
                name = "UNKNOWN";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

#endif //INFINIBAND_LIBIBVERBS_FORMAT_HPP
