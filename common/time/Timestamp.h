#ifndef MIDAS_TIMESTAMP_H
#define MIDAS_TIMESTAMP_H

#include <ostream>
#include <string>

using namespace std;

namespace midas {
class Timestamp {
public:
    // like 20180118 235959
    int cob{0};
    int time{0};
    int intradayMinute{0};

public:
    Timestamp() = default;
    Timestamp(int cob_, int time_) : cob(cob_), time(time_) { intradayMinute = intraday_minute(time_); }

    void init_from_intraday_minute(int date_, int intradayMinute_);

    std::string time2string() const;

    std::string cob2string() const;

    std::string to_string() const;

    void clear() { cob = time = intradayMinute = 0; }

    bool is_valid() const { return cob == 0; }

    static std::string time2string(int time);
    static std::string cob2string(int cob);
    /**
     * 20171112 and 23:59:59 to "2017-11-12 23:59:59"
     */
    static std::string to_string(int cob, int time);
    static int intraday_minute(int t);
    static int intraday_minute2hms(int intradayMinute);
};
bool operator<(const Timestamp& left, const Timestamp& right);
bool operator>(const Timestamp& left, const Timestamp& right);
bool operator<=(const Timestamp& left, const Timestamp& right);
bool operator>=(const Timestamp& left, const Timestamp& right);
bool operator==(const Timestamp& left, const Timestamp& right);
bool operator!=(const Timestamp& left, const Timestamp& right);

ostream& operator<<(ostream& os, const Timestamp& timestamp);
}

#endif
