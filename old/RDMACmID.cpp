//
// Created by jonas on 07.04.21.
//

#include "RDMACmID.hpp"

RDMACmID::RDMACmID(RDMAEventChannel &ec) {
    rdma_create_id(ec.get(), &rdmaId, nullptr, RDMA_PS_TCP);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = 0; // Choose port for us
    rdma_bind_addr(rdmaId, (struct sockaddr *) &addr);
    rdma_listen(rdmaId, 10);
    auto port = ntohs(rdma_get_src_port(rdmaId));
    fmt::print("Listening on port {}\n", port);
}

RDMACmID::~RDMACmID() {
    // TODO: "Users must destroy any QP associated with an rdma_cm_id before destroying the ID."
    rdma_destroy_id(rdmaId);
}
