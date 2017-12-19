#include "TradeSession.h"
#include "utils/VisualHelper.h"
#include "utils/convert/TimeHelper.h"
#include "utils/log/Log.h"

constexpr int oneDayMinute = 24 * 60;
constexpr int sessionMinuteThreshold = 2;

TradeSession::TradeSession(const string& str) {
    auto itr1 = str.find_first_of('-');
    if (itr1 != string::npos) {
        from = midas::intraday_time_HM(str.substr(0, itr1));
        to = midas::intraday_time_HM(str.substr(itr1 + 1));
        int fromHour = from / 100;
        int fromMinute = from % 100;
        int toHour = to / 100;
        int toMinute = to % 100;
        fromIntradayMinute = fromHour * 60 + fromMinute;
        toIntradayMinute = toHour * 60 + toMinute;

        if (to > from)
            sessionMinute = toIntradayMinute - fromIntradayMinute;
        else
            sessionMinute = toIntradayMinute - fromIntradayMinute + oneDayMinute;
    } else {
        MIDAS_LOG_ERROR("trade session init error: " << str);
    }
}

int TradeSession::minute() const { return sessionMinute; }

bool TradeSession::is_in_this_session(int intradayMinute) {
    if (to > from) {
        return fromIntradayMinute - sessionMinuteThreshold <= intradayMinute &&
               intradayMinute <= toIntradayMinute + sessionMinuteThreshold;
    } else {
        return (fromIntradayMinute - sessionMinuteThreshold <= intradayMinute && intradayMinute < oneDayMinute) ||
               (0 <= intradayMinute && intradayMinute < toIntradayMinute + sessionMinuteThreshold);
    }
}

int TradeSession::adjust_within_session(int startIntradayMinute, int scale) {
    if (startIntradayMinute + scale == fromIntradayMinute) {
        return fromIntradayMinute;
    } else if (startIntradayMinute == toIntradayMinute) {
        return toIntradayMinute - scale;
    } else {
        return startIntradayMinute;
    }
}

int TradeSessions::adjust_within_session(int startIntradayMinute, int scale) {
    return sessions[sessionIndex].adjust_within_session(startIntradayMinute, scale);
}

int TradeSessions::minute() const {
    int result{0};
    for (const auto& s : sessions) {
        result += s.minute();
    }
    return result;
}

/**
 * first check current session, if not in, then check next session
 * @param updateTime
 * @return
 */
bool TradeSessions::update_session(int intradayMinute) {
    if (sessionIndex >= 0) {
        if (sessions[sessionIndex].is_in_this_session(intradayMinute))
            return true;
        else if (sessionIndex + 1 < sessionCount && sessions[sessionIndex + 1].is_in_this_session(intradayMinute)) {
            sessionIndex += 1;
            return true;
        }
    }

    for (size_t i = 0; i < sessions.size(); ++i) {
        if (sessions[i].is_in_this_session(intradayMinute)) {
            sessionIndex = static_cast<int>(i);
            return true;
        }
    }
    sessionIndex = -1;
    return false;
}

void TradeSessions::add_session(const TradeSession& session) {
    sessions.push_back(session);
    sessionCount += 1;
}

ostream& operator<<(ostream& s, const TradeSessions& sessions) {
    s << sessions.sessions << " current index: " << sessions.sessionIndex << " minute: " << sessions.minute();
    return s;
}

ostream& operator<<(ostream& s, const TradeSession& session) {
    s << session.from << " <-> " << session.to << " " << session.minute();
    return s;
}
