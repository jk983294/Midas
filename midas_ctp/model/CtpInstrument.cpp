#include "CtpInstrument.h"

CtpInstrument::CtpInstrument(const string& _instrument, const TradeSessions& s) : instrument(_instrument), sessions(s) {
    candles15.set_session(s);
    candles30.set_session(s);
}

inline static bool is_volume_abnormal(int v) { return v <= 0 || v > 100000000; }

void CtpInstrument::update_tick(const MktDataPayload& payload) {
    ++updateCount;

    const CThostFtdcDepthMarketDataField& tick = payload.get_data();

    int ts;  // trade size
    if (tick.Volume == 0 || updateCount == 1) {
        ts = 0;
    } else {
        ts = tick.Volume - image.Volume;
    }

    if (ts > 0) {
        double newHigh = (tick.HighestPrice > image.HighestPrice ? tick.HighestPrice : -1);
        double newLow = (tick.LowestPrice < image.LowestPrice ? tick.LowestPrice : -1);
        candles15.update(tick, ts, newHigh, newLow);
        candles30.update(tick, ts, newHigh, newLow);
    }

    image = tick;
}

void CtpInstrument::load_historic_candle15(vector<CandleData>& candles) { candles15.init(candles); }

void CtpInstrument::book_stream(ostream& os) {
    const static int defaultPrice = -99999;
    os << "Price          |         Volume" << '\n'
       << left << setw(15) << (is_volume_abnormal(image.AskVolume5) ? defaultPrice : image.AskPrice5) << "|" << right
       << setw(15) << image.AskVolume5 << '\n'
       << "---------------|---------------" << '\n'
       << left << setw(15) << (is_volume_abnormal(image.BidVolume1) ? defaultPrice : image.BidPrice1) << "|" << right
       << setw(15) << image.BidVolume1;
}

void CtpInstrument::image_stream(ostream& os) {
    os << left << setw(15) << "e " << right << setw(15) << image.ExchangeID << '\n'
       << left << setw(15) << "id " << right << setw(15) << image.InstrumentID << '\n'
       << left << setw(15) << "tp " << right << setw(15) << image.LastPrice << '\n'
       << left << setw(15) << "lsp " << right << setw(15) << image.PreSettlementPrice << '\n'
       << left << setw(15) << "lccp " << right << setw(15) << image.PreClosePrice << '\n'
       << left << setw(15) << "lois " << right << setw(15) << image.PreOpenInterest << '\n'
       << left << setw(15) << "op " << right << setw(15) << image.OpenPrice << '\n'
       << left << setw(15) << "hp " << right << setw(15) << image.HighestPrice << '\n'
       << left << setw(15) << "lp " << right << setw(15) << image.LowestPrice << '\n'
       << left << setw(15) << "ts " << right << setw(15) << image.Volume << '\n'
       << left << setw(15) << "to " << right << setw(15) << image.Turnover << '\n'
       << left << setw(15) << "ois " << right << setw(15) << image.OpenInterest << '\n'
       << left << setw(15) << "cp " << right << setw(15) << image.ClosePrice << '\n'
       << left << setw(15) << "sp " << right << setw(15) << image.SettlementPrice << '\n'
       << left << setw(15) << "ulp " << right << setw(15) << image.UpperLimitPrice << '\n'
       << left << setw(15) << "llp " << right << setw(15) << image.LowerLimitPrice << '\n'
       << left << setw(15) << "pd " << right << setw(15) << image.PreDelta << '\n'
       << left << setw(15) << "cd " << right << setw(15) << image.CurrDelta << '\n'
       << left << setw(15) << "ut " << right << setw(15) << image.UpdateTime << '\n'
       << left << setw(15) << "um " << right << setw(15) << image.UpdateMillisec << '\n'
       << left << setw(15) << "bbp1 " << right << setw(15) << image.BidPrice1 << '\n'
       << left << setw(15) << "bbs1 " << right << setw(15) << image.BidVolume1 << '\n'
       << left << setw(15) << "bap1 " << right << setw(15) << image.AskPrice1 << '\n'
       << left << setw(15) << "bas1 " << right << setw(15) << image.AskVolume1 << '\n'
       << left << setw(15) << "avgp " << right << setw(15) << image.AveragePrice << '\n'
       << left << setw(15) << "ad " << right << setw(15) << image.ActionDay << '\n'
       << left << setw(15) << "updateCount " << right << setw(15) << updateCount << '\n';
}

ostream& operator<<(ostream& os, const CtpInstrument& instrument) {
    os << instrument.instrument << " updateCount: " << instrument.updateCount << '\n';
    os << "candles15:" << '\n' << instrument.candles15 << '\n';
    os << "candles30:" << '\n' << instrument.candles30 << '\n';
    return os;
}
