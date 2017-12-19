#ifndef MIDAS_CTP_INSTRUMENT_H
#define MIDAS_CTP_INSTRUMENT_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <net/disruptor/Payload.h>
#include <trade/TradeStatusManager.h>
#include <string>
#include <vector>
#include "CandleData.h"

using namespace std;

typedef midas::PayloadObject<CThostFtdcDepthMarketDataField> MktDataPayload;

class CtpInstrument {
public:
    long updateCount{0};
    string instrument;
    Candles candles15{CandleScale::Minute15};
    Candles candles30{CandleScale::Minute30};
    TradeSessions sessions;
    CThostFtdcDepthMarketDataField image;

public:
    CtpInstrument(const string& instrument, const TradeSessions& s);

    void book_stream(ostream& os);

    void image_stream(ostream& os);

    void update_tick(const MktDataPayload& tick);

    void load_historic_candle15(vector<CandleData>& candles);
};

ostream& operator<<(ostream& os, const CtpInstrument& instrument);

#endif
