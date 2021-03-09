/**
 * @file IBvContext.cpp.cc
 * @author ottojo
 * @date 2/24/21
 * Description here TODO
 */

#include <infiniband/verbs.h>
#include "IBvContext.hpp"
#include <gsl/gsl>

IBvContext::IBvContext(struct ibv_device *device) {
    context = ibv_open_device(device);
    Expects(context != nullptr);
}

IBvContext::~IBvContext() {
    ibv_close_device(context);
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
    if (error != 0) {
        // TODO: find out how to convert error codes to string
        throw std::runtime_error{strerror(error)};
    }
    return attr;
}
