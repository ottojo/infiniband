//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_CONNECTIONINFO_HPP
#define INFINIBAND_CONNECTIONINFO_HPP

#include <nlohmann/json.hpp>

struct ConnectionInfo {
    uint16_t localLID;
    uint32_t queuePairNumber;
    int packetSequenceNumber;
    uint32_t remoteKey;
    int vaddr;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConnectionInfo, localLID, queuePairNumber, packetSequenceNumber, remoteKey, vaddr)

#endif //INFINIBAND_CONNECTIONINFO_HPP
