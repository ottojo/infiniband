/**
 * @file Buffer.hpp
 * @author ottojo
 * @date 5/25/21
 * Description here TODO
 */

#ifndef INFINIBAND_BUFFER_HPP
#define INFINIBAND_BUFFER_HPP

#include <memory>
#include <cassert>
#include <infiniband/verbs.h>
#include <fmt/format.h>

template<typename T>
class Buffer {
    public:
        explicit Buffer(std::size_t size, ibv_pd *protectionDomain) :
                _size(size),
                _data(new T[size]),
                memoryRegionMetadata(
                        ibv_reg_mr(protectionDomain, _data.get(), size * sizeof(T),
                                   IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE)) {
            if (memoryRegionMetadata == nullptr) {
                throw std::runtime_error{fmt::format("Registering send memory region failed: {}", strerror(errno))};
            }
        };

        ~Buffer() {
            ibv_dereg_mr(memoryRegionMetadata);
        }

        Buffer(const Buffer &) = delete;

        Buffer(Buffer &&) noexcept = default;

        Buffer &operator=(const Buffer &) = delete;

        Buffer &operator=(Buffer &&) noexcept = default;

        T &at(std::ptrdiff_t i) {
            assert(i < _size);
            return _data[i];
        }

        const T &at(std::ptrdiff_t i) const {
            assert(i < _size);
            return _data[i];
        }

        T *data() {
            return _data.get();
        }

        ibv_mr *getMR() {
            return memoryRegionMetadata;
        }

        [[nodiscard]] std::size_t size() const {
            return _size;
        }

    private:
        std::size_t _size;
        std::unique_ptr<T[]> _data;
        gsl::owner<ibv_mr *> memoryRegionMetadata;
};


#endif //INFINIBAND_BUFFER_HPP
