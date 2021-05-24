//
// Created by jonas on 09.03.21.
//

#include <cstring>
#include "IBvException.hpp"

const char *IBvException::what() const noexcept {
    return message.c_str();
}

IBvException::IBvException(int error) : message{"Error " + std::to_string(error) + ": " + strerror(error)} {}
