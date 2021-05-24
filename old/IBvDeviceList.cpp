/**
 * @file IBvDeviceList.cpp.cc
 * @author ottojo
 * @date 2/24/21
 * Description here TODO
 */

#include <infiniband/verbs.h>
#include <stdexcept>
#include "IBvDeviceList.hpp"
#include <fmt/format.h>
#include <gsl/gsl>

IBvDeviceList::IBvDeviceList() : devices{ibv_get_device_list(&list_size)} {
    if (devices == nullptr) {
        throw std::runtime_error{"device list invalid"};
    }

    Ensures(size() == list_size);
}

IBvDeviceList::~IBvDeviceList() {
    ibv_free_device_list(devices);
}

IBvDeviceList::devicePtr IBvDeviceList::at(int i) {
    if (i < list_size) {
        return devices[i];
    } else {
        throw std::out_of_range{
                fmt::format(FMT_STRING("attempted to access device {} in list of size {}"), i, list_size)};
    }
}

IBvDeviceList::devicePtr IBvDeviceList::operator[](int i) {
    return at(i);
}

IBvDeviceList::iterator IBvDeviceList::begin() {
    return &devices[0];
}

IBvDeviceList::iterator IBvDeviceList::end() {
    return &devices[list_size];
}

