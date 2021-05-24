//
// Created by jonas on 09.03.21.
//

#include "IBvProtectionDomain.hpp"

IBvProtectionDomain::IBvProtectionDomain(IBvContext &context) :
        pd{ibv_alloc_pd(context.get())} {
    if (pd == nullptr) {
        throw std::runtime_error("Allocating protection domain failed");
    }
}

IBvProtectionDomain::~IBvProtectionDomain() {
    ibv_dealloc_pd(pd);
}

ibv_pd *IBvProtectionDomain::get() {
    return pd;
}
