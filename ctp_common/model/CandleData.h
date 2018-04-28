#ifndef MIDAS_CANDLE_DATA_H
#define MIDAS_CANDLE_DATA_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <string>
#include <vector>
#include "TradeSession.h"
#include "time/Timestamp.h"

using namespace std;

enum CandleScale { Minute1 = 1, Minute5 = 5, Minute15 = 15, Minute30 = 30, Hour1 = 60, Day1 = 24 * 60 };

class CandleData {
public:
    int tickCount{0};
    midas::Timestamp timestamp;
    double open{0};
    double high{0};
    double low{0};
    double close{0};
    double volume{0};  // trade size

public:
    CandleData() {}
    CandleData(int date, int time, double open, double high, double low, double close, double volume);

    void update_first_tick(int _date, int _intradayMinute, double tp, int ts, double newHigh, double newLow);

    void update_tick(double tp, int ts, double newHigh, double newLow);

    void update(const CandleData& data);
};

class Candles {
public:
    CandleScale scale;
    vector<CandleData> data;
    TradeSessions sessions;
    int historicDataCount{0};
    int totalBinCount{0};
    /**
     * if data[currentBinIndex].timestamp.cob == 0, then totalAvailableCount = currentBinIndex
     * else totalAvailableCount = currentBinIndex + 1
     */
    int currentBinIndex{0};
    long updateCount{0};  // how many tick updated so far, used for meters

public:
    Candles(CandleScale s = CandleScale::Minute15);

    void init(const vector<CandleData>& historicData, CandleScale historicScale = CandleScale::Minute15);

    void update(const CThostFtdcDepthMarketDataField& tick, int ts, double newHigh, double newLow);

    void set_session(const TradeSessions& ts) { sessions = ts; }

    int total_available_count() const {
        if (data[currentBinIndex].timestamp.cob != 0)
            return currentBinIndex + 1;
        else
            return currentBinIndex;
    }

    const CandleData& operator[](int i) const { return data[i]; }
};

ostream& operator<<(ostream& os, const CandleData& candle);
ostream& operator<<(ostream& os, const Candles& candles);

#endif
