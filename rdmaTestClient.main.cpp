//
// Created by jonas on 08.04.21.
//

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <rdma/rdma_cma.h>
#include <thread>
#include <opencv2/opencv.hpp>

#include "rdmaLib.hpp"
#include "RDMAClient.hpp"


std::promise<bool> programEnd;

std::unique_ptr<RDMAClient> client;

std::chrono::steady_clock::time_point sendTime;

void onReceive(const ibv_wc &wc, Buffer<char> b) {
    std::chrono::duration<double, std::milli> duration = std::chrono::steady_clock::now() - sendTime;
    fmt::print("onReceive after {}\n", duration);
    cv::Mat inputMatrix(HEIGHT, WIDTH, CV_8UC1, (void *) b.data());
    cv::imshow("circle", inputMatrix);
    while (cv::waitKey(1) != 27);
    cv::destroyWindow("circle");
    client->returnBuffer(std::move(b));
    programEnd.set_value(true);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fmt::print("Usage: {} <server-address> <server-port>\n", argv[0]);
        std::exit(1);
    }

    client = std::make_unique<RDMAClient>(argv[1], argv[2], BUFFER_SIZE, BUFFER_SIZE,
                                          [](const ibv_wc &wc, Buffer<char> b) {
                                              onReceive(wc, std::move(b));
                                          });

    {
        auto b = client->getSendBuffer();
        cv::Mat sendMatrix(HEIGHT, WIDTH, CV_8UC1, (void *) b.data());
        sendMatrix.setTo(cv::Scalar(255));
        cv::circle(sendMatrix, cv::Point(WIDTH / 2, HEIGHT / 2), HEIGHT / 2, cv::Scalar(0), 20);
        sendTime = std::chrono::steady_clock::now();
        fmt::print("Sending message!\n");
        client->send(std::move(b));
    }

    programEnd.get_future().wait();

    return 0;
}
