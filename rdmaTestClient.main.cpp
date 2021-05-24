//
// Created by jonas on 08.04.21.
//

#include <fmt/format.h>

#include <rdma/rdma_cma.h>
#include <thread>
#include <netdb.h>
#include <opencv2/opencv.hpp>

#include "rdmaLib.hpp"
#include "RDMAClient.hpp"

std::chrono::high_resolution_clock::time_point sendTime;

void onReceive(ClientConnection *conn) {
    cv::Mat inputMatrix(HEIGHT, WIDTH, CV_8UC1, (void *) conn->recv_region);

    cv::imshow("circle", inputMatrix);
    while (cv::waitKey(1) != 27);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fmt::print("Usage: {} <server-address> <server-port>\n", argv[0]);
        std::exit(1);
    }

    auto client = std::make_unique<RDMAClient>(argv[1], argv[2], BUFFER_SIZE,
                                               [](ClientConnection *conn) { onReceive(conn); });


    std::this_thread::sleep_for(std::chrono::hours{100000});

    return 0;
}
