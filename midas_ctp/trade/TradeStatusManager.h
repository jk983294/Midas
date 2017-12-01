#ifndef MIDAS_TRADE_STATUS_MANAGER_H
#define MIDAS_TRADE_STATUS_MANAGER_H

#include <model/CandleData.h>
#include <map>
#include <string>
#include <vector>
#include "utils/log/Log.h"

using namespace std;

class TradeSession {
public:
    int from, to;
    int fromIntradayMinute, toIntradayMinute;
    int sessionMinute;

public:
    TradeSession() = default;
    TradeSession(const string& str);

    /**
     * @return total minute in this session
     */
    int minute() const;

    bool is_in_this_session(int intradayMinute);

    int adjust_within_session(int startIntradayMinute, int scale);
};

class TradeSessions {
public:
    int sessionIndex{-1};
    int sessionCount{0};

public:
    friend ostream& operator<<(ostream& s, const TradeSessions& sessions);

    int minute() const;

    /**
     * update sessionIndex to current session
     * @return true if it is in trading hour
     */
    bool update_session(int intradayMinute);

    size_t intraday_bin_index(int minute);

    void add_session(const TradeSession& session);

    int adjust_within_session(int startIntradayMinute, int scale);

public:
    static int intraday_minute(const char* updateTime);

private:
    vector<TradeSession> sessions;
};

class TradeStatusManager {
public:
    map<string, TradeSessions> product2sessions;

public:
    void load_trade_session(const string& path);

    TradeSessions* get_session(const string& instrumentId);
};

ostream& operator<<(ostream& s, const TradeStatusManager& manager);
ostream& operator<<(ostream& s, const TradeSessions& sessions);
ostream& operator<<(ostream& s, const TradeSession& session);

#endif
