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

void onReceive(const ibv_wc &wc, Buffer<char> &b) {
    cv::Mat inputMatrix(HEIGHT, WIDTH, CV_8UC1, (void *) b.data());
    cv::imshow("circle", inputMatrix);
    while (cv::waitKey(1) != 27);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fmt::print("Usage: {} <server-address> <server-port>\n", argv[0]);
        std::exit(1);
    }

    std::unique_ptr<RDMAClient> client = std::make_unique<RDMAClient>(argv[1], argv[2], BUFFER_SIZE, BUFFER_SIZE,
                                                                      [&client](const ibv_wc &wc, Buffer<char> &&b) {
                                                                          onReceive(wc, b);
                                                                          client->returnBuffer(std::move(b));
                                                                      });

    {
        auto b = client->getBuffer();
        cv::Mat sendMatrix(HEIGHT, WIDTH, CV_8UC1, (void *) b.data());
        sendMatrix.setTo(cv::Scalar(255));
        cv::circle(sendMatrix, cv::Point(WIDTH / 2, HEIGHT / 2), HEIGHT / 2, cv::Scalar(0), 20);
        client->send(std::move(b));
    }
    std::this_thread::sleep_for(std::chrono::hours{100000});

    return 0;
}
