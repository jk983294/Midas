#ifndef MIDAS_UTILS_H
#define MIDAS_UTILS_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include "midas/MidasTick.h"

using namespace std;
using namespace midas;

inline void convert(char* dest, string src) { strcpy(dest, src.c_str()); }

inline CThostFtdcDepthMarketDataField convert(const MidasTick& tick) {
    CThostFtdcDepthMarketDataField field{0};
    convert(field.ExchangeID, tick["e"]);
    convert(field.InstrumentID, tick["id"]);
    field.LastPrice = tick["tp"];
    field.Volume = tick["ts"];
    field.PreSettlementPrice = tick["lsp"];
    field.PreClosePrice = tick["lccp"];
    field.PreOpenInterest = tick["lois"];
    field.OpenPrice = tick["op"];
    field.HighestPrice = tick["hp"];
    field.LowestPrice = tick["lp"];
    field.Turnover = tick["to"];
    field.OpenInterest = tick["ois"];
    field.ClosePrice = tick["cp"];
    field.SettlementPrice = tick["sp"];
    field.UpperLimitPrice = tick["ulp"];
    field.LowerLimitPrice = tick["llp"];
    field.PreDelta = tick["pd"];
    field.CurrDelta = tick["cd"];
    convert(field.UpdateTime, tick["ut"]);
    field.UpdateMillisec = tick["um"];
    field.BidPrice1 = tick["bbp1"];
    field.BidVolume1 = tick["bbs1"];
    field.AskPrice1 = tick["bap1"];
    field.AskVolume1 = tick["bas1"];
    field.AveragePrice = tick["avgp"];
    convert(field.TradingDay, tick["tpd"]);
    convert(field.ActionDay, tick["ad"]);
    return field;
}

#endif
