/**
 * @file utils.cpp
 * @author ottojo
 * @date 5/25/21
 * Description here TODO
 */

#include "utils.hpp"

#include <fmt/format.h>

FileDescriptorSets waitForFDs(const FileDescriptorSets &fds, int timeoutSec, int timeoutUsec) {

    while (true) {
        fd_set readSet, writeSet, exceptSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        FD_ZERO(&exceptSet);
        for (const auto &fd: fds.read) {
            FD_SET(fd, &readSet);
        }
        for (const auto &fd: fds.write) {
            FD_SET(fd, &writeSet);
        }
        for (const auto &fd: fds.except) {
            FD_SET(fd, &exceptSet);
        }
        timeval timeout{.tv_sec=timeoutSec, .tv_usec=timeoutUsec};

        int res = select(FD_SETSIZE, &readSet, &writeSet, &exceptSet, &timeout);

        if (res == -1) {
            throw std::runtime_error{fmt::format("Error during select: {}", strerror(errno))};
        }
        if (res == 0) {
            fmt::print("eventloop\n");
            continue;
        }

        FileDescriptorSets ret;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &readSet)) {
                ret.read.push_back(i);
            }
            if (FD_ISSET(i, &writeSet)) {
                ret.write.push_back(i);
            }
            if (FD_ISSET(i, &exceptSet)) {
                ret.except.push_back(i);
            }
        }
        return ret;
    }
}
