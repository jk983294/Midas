#ifndef MIDAS_CTP_DEF_H
#define MIDAS_CTP_DEF_H

enum CtpOrderDirection { Long, Short };
enum CtpOrderType { Market, Limit, Condition };

constexpr bool BuyFlag = true;
constexpr bool SellFlag = false;
constexpr double InitAsset = 1000000.0;

#endif
