cmake_minimum_required(VERSION 3.16)
project(infiniband)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")


find_package(fmt REQUIRED)
find_package(verbs REQUIRED)
find_package(Microsoft.GSL CONFIG)
find_package(Boost REQUIRED) # stlinterfaces
find_library(RDMACM rdmacm REQUIRED)
find_package(OpenCV REQUIRED)

#add_executable(infiniband main.cpp IBvContext.cpp IBvDeviceList.cpp libibverbs_format.hpp IBvProtectionDomain.cpp IBvCompletionQueue.cpp IBvCompletionQueue.hpp IBvCompletionEventChannel.cpp IBvCompletionEventChannel.hpp QueuePair.cpp QueuePair.hpp IBvException.cpp IBvException.hpp IBvMemoryRegion.hpp RDMAEventChannel.cpp RDMAEventChannel.h IBConnection.cpp IBConnection.h RDMACmID.cpp RDMACmID.hpp)
#target_link_libraries(infiniband PRIVATE IBVerbs::verbs fmt::fmt Microsoft.GSL::GSL pthread rdmacm)
#target_compile_options(infiniband PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-field-initializers)

add_executable(rdmaTestServer rdmaTestServer.main.cpp rdmaLib.cpp RDMAServer.cpp utils.cpp BreakableEventLoop.cpp BreakableFDWait.cpp BufferSet.cpp CompletionPoller.cpp)
target_include_directories(rdmaTestServer PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(rdmaTestServer PRIVATE fmt::fmt IBVerbs::verbs pthread rdmacm ${OpenCV_LIBS})


add_executable(rdmaTestClient rdmaTestClient.main.cpp rdmaLib.cpp RDMAClient.cpp utils.cpp BreakableEventLoop.cpp BreakableFDWait.cpp BufferSet.cpp CompletionPoller.cpp)
target_link_libraries(rdmaTestClient PRIVATE fmt::fmt IBVerbs::verbs pthread rdmacm ${OpenCV_LIBS})
