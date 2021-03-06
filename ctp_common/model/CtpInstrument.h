#ifndef MIDAS_CTP_INSTRUMENT_H
#define MIDAS_CTP_INSTRUMENT_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <net/disruptor/Payload.h>
#include <trade/TradeStatusManager.h>
#include <memory>
#include <string>
#include <vector>
#include "CandleData.h"
#include "strategy/StrategyBase.h"

using namespace std;

typedef midas::PayloadObject<CThostFtdcDepthMarketDataField> MktDataPayload;

class CtpInstrument {
public:
    bool isMasterContract{false};
    long updateCount{0};
    string id;
    string instrumentName;
    string productName;
    Candles candles15{CandleScale::Minute15};
    Candles candles30{CandleScale::Minute30};
    TradeSessions sessions;
    std::atomic<double> tp{0};
    CThostFtdcDepthMarketDataField image;
    std::shared_ptr<CThostFtdcInstrumentField> info;
    std::unique_ptr<StrategyBase> strategy;

public:
    CtpInstrument(const string& instrument, const TradeSessions& s);

    void book_stream(ostream& os);

    void image_stream(ostream& os);

    void update_tick(const MktDataPayload& tick);

    void load_historic_candle(vector<CandleData>& candles, CandleScale historicScale = CandleScale::Minute15);

    const Candles& get_candle_reference(CandleScale scale = CandleScale::Minute15) const;

    void market_price(double p);
    double market_price();
};

ostream& operator<<(ostream& os, const CtpInstrument& instrument);

#endif
