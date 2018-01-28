#ifndef MIDAS_TRADE_SESSION_H
#define MIDAS_TRADE_SESSION_H

#include <string>
#include <vector>

using namespace std;

class TradeSession {
public:
    int from, to;
    int fromIntradayMinute, toIntradayMinute;
    int sessionMinute;

public:
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
    int sessionCount{0};
    int sessionIndex{-1};

public:
    friend ostream& operator<<(ostream& s, const TradeSessions& sessions);

    int minute() const;

    /**
     * update sessionIndex to current session
     * @return true if it is in trading hour
     */
    bool update_session(int intradayMinute);

    void add_session(const TradeSession& session);

    int adjust_within_session(int startIntradayMinute, int scale);

public:
    static int intraday_minute(const char* updateTime);
    static int intraday_minute(int hms);
    static int intraday_minute2hms(int intradayMinute);

private:
    vector<TradeSession> sessions;
};

ostream& operator<<(ostream& s, const TradeSessions& sessions);
ostream& operator<<(ostream& s, const TradeSession& session);

#endif
