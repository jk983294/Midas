#ifndef MIDAS_MDDEFS_H
#define MIDAS_MDDEFS_H

#include <cstdint>

namespace midas {
constexpr uint16_t ExchangeNone = 0x0000;
constexpr uint16_t ExchangeCFFEX = 0x0001;  // 中国金融交易所
constexpr uint16_t ExchangeCZCE = 0x0002;   // 郑州商品交易所
constexpr uint16_t ExchangeDCE = 0x0003;    // 大连商品交易所
constexpr uint16_t ExchangeINE = 0x0004;    // 上海国际能源交易中心股份有限公司
constexpr uint16_t ExchangeSHFE = 0x0005;   // 上海期货交易所

constexpr int64_t PriceBlank = INT64_MIN;
constexpr uint8_t DefaultPriceScale = 4;
constexpr uint32_t DefaultLotSize = 100;
}

#endif
