/**
 * @file IBvDeviceList.hpp.h
 * @author ottojo
 * @date 2/24/21
 * Description here TODO
 */

#ifndef INFINIBAND_IBVDEVICELIST_HPP
#define INFINIBAND_IBVDEVICELIST_HPP

#include <boost/stl_interfaces/view_interface.hpp>
#include <ranges>

class IBvDeviceList : public boost::stl_interfaces::view_interface<IBvDeviceList> {
    public:

        using devicePtr = struct ibv_device *;
        using iterator = devicePtr const *;

        [[nodiscard]] iterator begin();

        [[nodiscard]] iterator end();

        IBvDeviceList();

        ~IBvDeviceList();

        IBvDeviceList(const IBvDeviceList &) = delete;

        devicePtr at(int i);

        devicePtr operator[](int i);

    private:
        int list_size = 0;
        devicePtr *const devices;
};

#endif //INFINIBAND_IBVDEVICELIST_HPP
