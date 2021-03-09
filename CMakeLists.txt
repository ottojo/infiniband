cmake_minimum_required(VERSION 3.19)
project(infiniband)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")


find_package(fmt REQUIRED)
find_package(verbs REQUIRED)
find_package(Microsoft.GSL CONFIG)
find_package(Boost REQUIRED) # stlinterfaces, beast

add_executable(infiniband main.cpp IBvContext.cpp IBvDeviceList.cpp libibverbs_format.hpp IBvProtectionDomain.cpp IBvCompletionQueue.cpp IBvCompletionQueue.hpp IBvCompletionEventChannel.cpp IBvCompletionEventChannel.hpp IBvQueuePair.cpp IBvQueuePair.hpp IBvException.cpp IBvException.hpp IBvMemoryRegion.hpp httpClient.cpp httpClient.hpp ConnectionInfo.cpp ConnectionInfo.hpp httpServer.cpp httpServer.hpp)
target_link_libraries(infiniband IBVerbs::verbs fmt::fmt Microsoft.GSL::GSL)
target_compile_options(infiniband PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-field-initializers)
target_include_directories(infiniband PRIVATE SYSTEM external/json/include)
