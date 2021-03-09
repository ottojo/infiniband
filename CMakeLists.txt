cmake_minimum_required(VERSION 3.19)
project(infiniband)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")


find_package(fmt REQUIRED)
find_package(verbs REQUIRED)
find_package(Microsoft.GSL CONFIG)
find_package(Boost REQUIRED) # stlinterfaces

add_executable(infiniband main.cpp IBvContext.cpp IBvDeviceList.cpp libibverbs_format.hpp)
target_link_libraries(infiniband IBVerbs::verbs fmt::fmt Microsoft.GSL::GSL)
