#ifndef MIDAS_MD_EXCHANGE_H
#define MIDAS_MD_EXCHANGE_H

#include <cstdint>
#include "MdDefs.h"

namespace midas {
class MdExchange {
public:
    MdExchange(uint16_t exchange_, uint8_t exchangeDepth_, uint16_t bytesPerProduct_, uint16_t offsetBytesBid_,
               uint16_t offsetBytesAsk_)
        : exchange(exchange_),
          bytesPerProduct(bytesPerProduct_),
          offsetBytesBid(offsetBytesBid_),
          offsetBytesAsk(offsetBytesAsk_),
          exchangeDepth(exchangeDepth_) {}

    MdExchange() = default;

    MdExchange(MdExchange const&) = default;

    MdExchange& operator=(MdExchange const&) = default;

    ~MdExchange() = default;

    uint16_t exchange{ExchangeNone};
    uint16_t bytesPerProduct{0};
    uint16_t offsetBytesBid{0};
    uint16_t offsetBytesAsk{0};
    uint8_t exchangeDepth{0};
};
}

#endif
