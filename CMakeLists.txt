cmake_minimum_required(VERSION 3.16)
project(infiniband)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1 -g")

find_package(fmt REQUIRED)
find_package(verbs REQUIRED)
find_package(Microsoft.GSL CONFIG)
find_library(RDMACM rdmacm REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Backward REQUIRED)
find_library(BFD bfd REQUIRED)
find_library(UNWIND unwind REQUIRED)

set(SOURCES_COMMON
        rdmaLib.cpp
        utils.cpp
        BreakableEventLoop.cpp
        BreakableFDWait.cpp
        BufferSet.cpp
        CompletionPoller.cpp
        backward.cpp
        )

set(SOURCES_SERVER
        ${SOURCES_COMMON}
        RDMAServer.cpp
        rdmaTestServer.main.cpp
        )

add_executable(rdmaTestServer ${SOURCES_SERVER})
target_include_directories(rdmaTestServer PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(rdmaTestServer PRIVATE fmt::fmt IBVerbs::verbs pthread rdmacm ${OpenCV_LIBS} Backward::Backward)

set(SOURCES_CLIENT
        ${SOURCES_COMMON}
        RDMAClient.cpp
        rdmaTestClient.main.cpp
        )

add_executable(rdmaTestClient ${SOURCES_CLIENT})
target_link_libraries(rdmaTestClient PRIVATE fmt::fmt IBVerbs::verbs pthread rdmacm ${OpenCV_LIBS} Backward::Backward)
