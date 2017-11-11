#include "InstrumentStats.h"

InstrumentStats::InstrumentStats(const string& instrument) : instrument(instrument) {}

void InstrumentStats::update(const MidasTick& tick) {
    ++updateCount;

    int actionDay = tick["ad"];
    string updateTime = tick["ut"];
    time_t ut = time_t_from_ymdhms(actionDay, midas::intraday_time_from_separator_string(updateTime));

    if (tradingTimes.empty() || (ut - (tradingTimes.end() - 1)->to > 10 * 60)) {
        tradingTimes.push_back({ut});
    } else {
        (tradingTimes.end() - 1)->update(ut);
    }
    //    cout << actionDay << '\t' << updateTime << '\t' << updateTime1 << endl;
}

void InstrumentStats::print() const {
    cout << instrument << " " << updateCount << " ";
    for (auto& session : tradingTimes) {
        if (session.span() < 30 * 60) continue;
        cout << session << " ";
    }
    cout << endl;
}

TradingSession::TradingSession(time_t ut) : from(ut), to(ut) {}

ostream& operator<<(ostream& os, const TradingSession& session) {
    os << "[" << midas::time_t2string(session.from) << " " << midas::time_t2string(session.to) << " "
       << session.updateCount << "]";
    return os;
}
