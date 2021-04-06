cmake_minimum_required(VERSION 3.16)
project(infiniband)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")


find_package(fmt REQUIRED)
find_package(verbs REQUIRED)
find_package(Microsoft.GSL CONFIG)
find_package(Boost REQUIRED) # stlinterfaces
find_library(RDMACM rdmacm REQUIRED)

add_executable(infiniband main.cpp IBvContext.cpp IBvDeviceList.cpp libibverbs_format.hpp IBvProtectionDomain.cpp IBvCompletionQueue.cpp IBvCompletionQueue.hpp IBvCompletionEventChannel.cpp IBvCompletionEventChannel.hpp IBvQueuePair.cpp IBvQueuePair.hpp IBvException.cpp IBvException.hpp IBvMemoryRegion.hpp RDMAEventChannel.cpp RDMAEventChannel.h IBConnection.cpp IBConnection.h)
target_link_libraries(infiniband IBVerbs::verbs fmt::fmt Microsoft.GSL::GSL pthread rdmacm)
target_compile_options(infiniband PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-field-initializers)
target_include_directories(infiniband SYSTEM PRIVATE external/json/include)
