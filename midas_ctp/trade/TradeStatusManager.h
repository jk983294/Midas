#ifndef MIDAS_TRADE_STATUS_MANAGER_H
#define MIDAS_TRADE_STATUS_MANAGER_H

#include <map>
#include "model/TradeSession.h"

using namespace std;

class TradeStatusManager {
public:
    map<string, TradeSessions> product2sessions;

public:
    void load_trade_session(const string& path);

    TradeSessions* get_session(const string& instrumentId);
};

ostream& operator<<(ostream& s, const TradeStatusManager& manager);

#endif
