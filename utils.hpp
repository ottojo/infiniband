/**
 * @file utils.hpp
 * @author ottojo
 * @date 5/25/21
 * Description here TODO
 */

#ifndef INFINIBAND_UTILS_HPP
#define INFINIBAND_UTILS_HPP

#include <vector>

struct FileDescriptorSets {
    std::vector<int> read;
    std::vector<int> write;
    std::vector<int> except;
};

FileDescriptorSets waitForFDs(const FileDescriptorSets &fds, int timeoutSec, int timeoutUsec);

template<typename T, typename E>
bool contains(const T &v, const E &x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}

#endif //INFINIBAND_UTILS_HPP
