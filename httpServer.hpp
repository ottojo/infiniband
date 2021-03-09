//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_HTTPSERVER_HPP
#define INFINIBAND_HTTPSERVER_HPP


#include "ConnectionInfo.hpp"

ConnectionInfo serveInfo(const std::string &port, const std::string &path, ConnectionInfo myInfo);


#endif //INFINIBAND_HTTPSERVER_HPP
