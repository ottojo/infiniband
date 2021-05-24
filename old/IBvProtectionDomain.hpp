//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_IBVPROTECTIONDOMAIN_HPP
#define INFINIBAND_IBVPROTECTIONDOMAIN_HPP

#include <infiniband/verbs.h>
#include <gsl/gsl>
#include "IBvContext.hpp"


class IBvProtectionDomain {
    public:
        explicit IBvProtectionDomain(IBvContext &context);

        IBvProtectionDomain(const IBvProtectionDomain &) = delete;

        IBvProtectionDomain &operator=(const IBvProtectionDomain &) = delete;

        ~IBvProtectionDomain();

        ibv_pd *get();

    private:
        gsl::owner<ibv_pd *> pd;
};


#endif //INFINIBAND_IBVPROTECTIONDOMAIN_HPP
