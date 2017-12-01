#ifndef MIDAS_CANDLE_DATA_H
#define MIDAS_CANDLE_DATA_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <vector>
#include "trade/TradeStatusManager.h"

using namespace std;

class TradeSessions;

enum CandleScale { Minute1 = 1, Minute5 = 5, Minute15 = 15, Minute30 = 30, Hour1 = 60, Day1 = 24 * 60 };

class CandleData {
public:
    int tickCount{0};
    int date{0};
    int time{0};
    int intradayMinute{0};
    int volume{0};
    double open{0};
    double close{0};
    double high{0};
    double low{0};

public:
    CandleData() {}
    CandleData(int date, int time, double open, double close, double high, double low);

    void update_first_tick(int _date, int _intradayMinute, double tp, int ts, double newHigh, double newLow);

    void update_tick(double tp, int ts, double newHigh, double newLow);

    int time_diff(int newDate, int newIntradayMinute);
};

class Candles {
public:
    CandleScale scale;
    vector<CandleData> data;
    TradeSessions* pSessions{nullptr};  // not the owner, only query boundary from sessions
    size_t historicDataCount{0};
    size_t totalBinCount{0};
    size_t currentBinIndex{0};
    long updateCount{0};

public:
    Candles(CandleScale s = CandleScale::Minute15);

    void init(vector<CandleData>& historicData);

    void update(const CThostFtdcDepthMarketDataField& tick, int ts, double newHigh, double newLow);

    void set_session_pointer(TradeSessions* p) { pSessions = p; }
};

ostream& operator<<(ostream& os, const CandleData& candle);
ostream& operator<<(ostream& os, const Candles& candles);

#endif
