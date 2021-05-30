/**
 * @file BufferSet.cpp
 * @author ottojo
 * @date 5/28/21
 * Description here TODO
 */

#include "BufferSet.hpp"

void BufferSet::returnBuffer(Buffer<char> &&b) {
    if (b.size() == recv_size) {
        receiveBufferPool.emplace(std::move(b));
    } else if (b.size() == send_size) {
        sendBufferPool.emplace(std::move(b));
    } else {
        throw std::runtime_error{
                fmt::format("Can not return buffer of size {}, we only have pools for sizes {} and {}", b.size(),
                            recv_size, send_size)};
    }
}

std::optional<Buffer<char>> BufferSet::getSendBuffer() {
    if (sendBufferPool.empty()) {
        return std::nullopt;
    }

    auto b = std::move(sendBufferPool.front());
    sendBufferPool.pop();
    return b;
}

std::optional<Buffer<char>> BufferSet::getRecvBuffer() {
    if (receiveBufferPool.empty()) {
        return std::nullopt;
    }

    auto b = std::move(receiveBufferPool.front());
    receiveBufferPool.pop();
    return b;
}

std::size_t BufferSet::getSendSize() const {
    return send_size;
}

std::size_t BufferSet::getRecvSize() const {
    return recv_size;
}

BufferSet::BufferSet(std::size_t sendSize, std::size_t recvSize) : recv_size(recvSize), send_size(sendSize) {}

void BufferSet::markInFlightSend(Buffer<char> &&b) {
    inFlightSendBuffers.emplace(b.data(), std::move(b));
}

void BufferSet::markInFlightRecv(Buffer<char> &&b) {
    inFlightReceiveBuffers.emplace(b.data(), std::move(b));
}

Buffer<char> BufferSet::findReceiveBuffer(char *key) {
    assert(inFlightReceiveBuffers.contains(key));
    auto buffer = std::move(inFlightReceiveBuffers.at(key));
    inFlightReceiveBuffers.erase(key);
    return buffer;
}

Buffer<char> BufferSet::findSendBuffer(char *key) {
    assert(inFlightSendBuffers.contains(key));
    auto buffer = std::move(inFlightSendBuffers.at(key));
    inFlightSendBuffers.erase(key);
    return buffer;
}

void BufferSet::clearBuffers() {
    {
        decltype(sendBufferPool) empty;
        std::swap(sendBufferPool, empty);
    }
    {
        decltype(receiveBufferPool) empty;
        std::swap(receiveBufferPool, empty);
    }
}
