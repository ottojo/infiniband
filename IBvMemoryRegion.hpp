//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_IBVMEMORYREGION_HPP
#define INFINIBAND_IBVMEMORYREGION_HPP


#include <vector>
#include <infiniband/verbs.h>
#include "IBvProtectionDomain.hpp"

template<typename T>
class IBvMemoryRegion {
    public:
        explicit IBvMemoryRegion(std::size_t size, IBvProtectionDomain &pd) :
                s(size),
                d(new T[size]),
                mr(ibv_reg_mr(
                           pd.get(),
                           static_cast<void *>(d.get()),
                           size * sizeof(T),
                           IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_WRITE)) {
            if (mr == nullptr) {
                throw std::runtime_error("Registering memory region failed");
            }
        };

        IBvMemoryRegion(const IBvMemoryRegion &) = delete;

        IBvMemoryRegion &operator=(const IBvMemoryRegion &) = delete;

        ~IBvMemoryRegion() {
            ibv_dereg_mr(mr);
        }

        std::shared_ptr<T[]> data() {
            return d;
        }

        std::shared_ptr<const T[]> data() const {
            return d;
        }

        [[nodiscard]] std::size_t size() const {
            return s;
        }

        struct ibv_mr *get() {
            return mr;
        }

    private:
        std::size_t s;
        std::shared_ptr<T[]> d;
        struct ibv_mr *mr;
};


#endif //INFINIBAND_IBVMEMORYREGION_HPP
