//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_HTTPCLIENT_HPP
#define INFINIBAND_HTTPCLIENT_HPP

#include "ConnectionInfo.hpp"

ConnectionInfo exchangeInfo(const std::string &host, const std::string &port, const std::string &path,
                            ConnectionInfo myInfo);

#endif //INFINIBAND_HTTPCLIENT_HPP
