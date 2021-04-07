//
// Created by jonas on 07.04.21.
//

#ifndef INFINIBAND_RDMACMID_HPP
#define INFINIBAND_RDMACMID_HPP

#include <rdma/rdma_cma.h>
#include <gsl/gsl>
#include "RDMAEventChannel.h"

class RDMACmID {
    public:
        explicit RDMACmID(RDMAEventChannel &ev);

        ~RDMACmID();

        RDMACmID(const RDMACmID &) = delete;

        RDMACmID &operator=(const RDMACmID &) = delete;

    private:
        gsl::owner<rdma_cm_id *> rdmaId = nullptr;
};


#endif //INFINIBAND_RDMACMID_HPP
