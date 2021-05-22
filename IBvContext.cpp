/**
 * @file IBvContext.cpp.cc
 * @author ottojo
 * @date 2/24/21
 * Description here TODO
 */

#include <gsl/gsl>
#include <infiniband/verbs.h>
#include "IBvContext.hpp"
#include "IBvException.hpp"

IBvContext::IBvContext(struct ibv_device *device) {
    context = ibv_open_device(device);
    Expects(context != nullptr);
}

IBvContext::IBvContext(struct ibv_context *c) : context(c) {}

IBvContext::~IBvContext() {
    // ibv_close_device(Context); // TODO FIX: This class is currently used both as the owner of a Context and as a wrapper
    //                                around a Context we get from librdmacm. Maybe the first usecase is not required?
}

struct ibv_context *IBvContext::get() {
    return context;
}

struct ibv_device_attr IBvContext::queryAttributes() {
    struct ibv_device_attr attributes{};
    ibv_query_device(context, &attributes);
    return attributes;
}

struct ibv_port_attr IBvContext::queryPort(int port) {
    struct ibv_port_attr attr{};
    auto error = ibv_query_port(context, port, &attr);
    throwIfError(error);
    return attr;
}
