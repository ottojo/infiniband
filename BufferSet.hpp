/**
 * @file BufferSet.hpp
 * @author ottojo
 * @date 5/28/21
 * Description here TODO
 */

#ifndef INFINIBAND_BUFFERSET_HPP
#define INFINIBAND_BUFFERSET_HPP

#include <queue>
#include <map>
#include "Buffer.hpp"
#include <optional>

class BufferSet {
    public:

        BufferSet(std::size_t sendSize, std::size_t recvSize);

        void returnBuffer(Buffer<char> &&b);

        std::optional<Buffer<char>> getSendBuffer();


        [[nodiscard]] std::size_t getSendSize() const;

        [[nodiscard]] std::size_t getRecvSize() const;

    protected:
        std::optional<Buffer<char>> getRecvBuffer();

        void markInFlightSend(Buffer<char> &&b);

        void markInFlightRecv(Buffer<char> &&b);

        Buffer<char> findReceiveBuffer(char *key);

        Buffer<char> findSendBuffer(char *key);

        void clearBuffers();


    private:
        std::queue<Buffer<char>> sendBufferPool;
        std::map<char *, Buffer<char>> inFlightSendBuffers;
        std::queue<Buffer<char>> receiveBufferPool;
        std::map<char *, Buffer<char>> inFlightReceiveBuffers;

        const std::size_t recv_size;
        const std::size_t send_size;
};


#endif //INFINIBAND_BUFFERSET_HPP
