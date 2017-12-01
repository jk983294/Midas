#include "InstrumentStats.h"

InstrumentStats::InstrumentStats(const string& instrument) : instrument(instrument) {}

void InstrumentStats::update(const CThostFtdcDepthMarketDataField& tick) {
    ++updateCount;
    maxTradeVolume = max(maxTradeVolume, tick.Volume);

    update_trading_hour(tick);
}

void InstrumentStats::update_trading_hour(const CThostFtdcDepthMarketDataField& tick) {
    time_t ut = time_t_from_ymdhms(midas::cob(tick.ActionDay), midas::intraday_time_HMS(tick.UpdateTime));

    if (tradingTimes.empty() || (ut - (tradingTimes.end() - 1)->to > 10 * 60)) {
        tradingTimes.push_back({ut});
    } else {
        (tradingTimes.end() - 1)->update(ut);
    }
}

void InstrumentStats::print_trading_hour() const {
    cout << instrument << " updateCount: " << updateCount << " max ts:" << maxTradeVolume << " ";
    for (auto& session : tradingTimes) {
        if (session.span() < 30 * 60) continue;
        cout << session << " ";
    }
    cout << '\n';
}

TradingSession::TradingSession(time_t ut) : from(ut), to(ut) {}

ostream& operator<<(ostream& os, const TradingSession& session) {
    os << "[" << midas::time_t2string(session.from) << " " << midas::time_t2string(session.to) << " "
       << session.updateCount << "]";
    return os;
}
