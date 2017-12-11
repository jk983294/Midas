#ifndef MIDAS_HEADER_H
#define MIDAS_HEADER_H

#include <netinet/in.h>
#include <stdint.h>
#include <cstring>
#include "midas/MidasConstants.h"

namespace midas {

#pragma pack(1)
typedef struct {
    uint16_t len;
    uint32_t second;
    uint32_t usecond;
    uint8_t flag;
    uint8_t group;
    uint32_t sequence;
} MidasHeader;
#pragma pack()

inline uint32_t midas_header_size() { return sizeof(MidasHeader); }

/**
 * write midas header into buffer
 * 0 - 1    msg length (total exclude length field)
 * 2 - 5    time second
 * 6 - 9    time usecond
 * 10       255
 * 11       group
 * 12 - 15  msg sequence number
 * @return header size
 */
inline int write_midas_header(char* buffer, uint16_t msgLen, uint32_t timeSec, uint32_t timeUsec, uint8_t group,
                              uint32_t seq) {
    uint16_t len = htons(msgLen + sizeof(MidasHeader) - 2);
    std::memcpy(buffer, &len, 2);
    uint32_t second = htonl(timeSec);
    std::memcpy(buffer + 2, &second, 4);
    uint32_t usecond = htonl(timeUsec);
    std::memcpy(buffer + 6, &usecond, 4);
    *(buffer + 10) = 0xFF;
    *(buffer + 11) = group;
    uint32_t seqNo = htonl(seq);
    std::memcpy(buffer + 12, &seqNo, 4);
    return sizeof(MidasHeader);
}
}

#endif  // MIDAS_HEADER_H
