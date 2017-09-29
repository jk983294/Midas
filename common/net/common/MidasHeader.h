#ifndef MIDAS_HEADER_H
#define MIDAS_HEADER_H

#include <netinet/in.h>
#include <stdint.h>
#include <cstring>

namespace midas {

const int MIDAS_HEADER_SIZE = 14;

/**
 * write midas header into buffer
 * 0 - 1    msg length (total exclude length field)
 * 2 - 5    time second
 * 6 - 9    time usecond
 * 10       255
 * 11       group
 * 12       format
 * 13       version v1
 * @param buffer
 * @param msgLen
 * @param timeSec
 * @param timeUsec
 * @param group
 * @param format
 * @return header size
 */
inline int write_midas_header(char* buffer, uint32_t msgLen, uint32_t timeSec, uint32_t timeUsec, uint8_t group,
                              uint8_t format) {
    uint16_t len = htons(msgLen + MIDAS_HEADER_SIZE - 2);
    std::memcpy(buffer, &len, 2);
    uint32_t second = htonl(timeSec);
    std::memcpy(buffer + 2, &second, 4);
    uint32_t usecond = htonl(timeUsec);
    std::memcpy(buffer + 6, &usecond, 4);

    *(buffer + 10) = 0xFF;
    *(buffer + 11) = group;
    *(buffer + 12) = format;
    *(buffer + 13) = 1;
    return MIDAS_HEADER_SIZE;
}
}

#endif  // MIDAS_HEADER_H
