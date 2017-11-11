#ifndef MIDAS_TOOLS_INSTRUMENTS_TATS_H
#define MIDAS_TOOLS_INSTRUMENTS_TATS_H

#include <utility>
#include <vector>
#include "midas/MidasTick.h"

using namespace std;
using namespace midas;

class TradingSession {
public:
    time_t from;
    time_t to;
    int updateCount{1};

public:
    TradingSession(time_t from);
    time_t span() const { return to - from; }
    void update(time_t ut) {
        ++updateCount;
        to = ut;
    }
};

ostream& operator<<(ostream& s, const TradingSession& session);

class InstrumentStats {
public:
    vector<TradingSession> tradingTimes;
    string instrument;
    int updateCount{0};

public:
    InstrumentStats() {}
    InstrumentStats(const string& instrument);

    void update(const MidasTick& tick);

    void print() const;
};

#endif
