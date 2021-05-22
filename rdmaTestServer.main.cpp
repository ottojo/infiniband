//
// Created by jonas on 08.04.21.
//

#include "fmt/format.h"
#include <rdma/rdma_cma.h>
#include <thread>
#include <netinet/in.h>
#include <opencv2/opencv.hpp>

#include "rdmaLib.hpp"
#include "RDMAServer.hpp"
#include <signal.h>


volatile bool done = false;

void intHandler(int sig) {
    fmt::print("Signal received! {}\n", sig);
    done = true;
}


void receiveCB(RDMAServer &server, ServerConnection &conn) {

    auto start = std::chrono::high_resolution_clock::now();
    cv::Mat inputMatrix(HEIGHT, WIDTH, CV_8UC1, (void *) conn.recv_region.data());
    cv::Mat outputMatrix(HEIGHT, WIDTH, CV_8UC1, (void *) conn.send_region.data());
    cv::GaussianBlur(inputMatrix, outputMatrix, cv::Size(0, 0), 5);
    auto time = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start);
    fmt::print("Calculation took {}ms\n", time.count());

    server.send(conn);
}


int main(int argc, char *argv[]) {

    RDMAServer s(42069, BUFFER_SIZE, BUFFER_SIZE, []() {}, &receiveCB);
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

    while (not done) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }

    return 0;
}
