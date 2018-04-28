#ifndef MIDAS_CTP_DEF_H
#define MIDAS_CTP_DEF_H

#include <cstdint>
#include <string>

enum CtpDirection { Long, Short };
enum CtpOrderType { Market, Limit, Condition };

constexpr bool BuyFlag = true;
constexpr bool SellFlag = false;
constexpr double InitAsset = 1000000.0;

constexpr int64_t signalValueBuyThreshold = 100;
constexpr int64_t signalValueSellThreshold = -100;
constexpr int64_t signalClearLong = -42;
constexpr int64_t signalClearShort = 42;
constexpr int64_t signalValueNo = 0;
constexpr double commissionPerLot = 10;  // 10 RMB per lot
constexpr double commissionRate = 0.00015;
constexpr double maxLeverage = 2;  // total leverage = (cash + instrumentValue) / (cash + margin)
constexpr int splitFactor = 5;     // at one time, at most splitFactor instruments can be held

const std::string allStrategies{"TMaStrategy|TBiMaStrategy"};

#endif
