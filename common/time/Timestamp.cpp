#include <iomanip>
#include "Timestamp.h"

namespace midas {

void Timestamp::init_from_intraday_minute(int date_, int intradayMinute_) {
    cob = date_;
    intradayMinute = intradayMinute_;
    time = intraday_minute2hms(intradayMinute_);
}

std::string Timestamp::time2string(int intradayTime) {
    int second = intradayTime % 100;
    intradayTime /= 100;
    int minute = intradayTime % 100;
    int hour = intradayTime / 100;

    char buffer[12];
    std::snprintf(buffer, sizeof buffer, "%02u:%02u:%02u", hour, minute, second);
    return std::string(buffer);
}

std::string Timestamp::time2string() const { return time2string(time); }

std::string Timestamp::cob2string(int cob) {
    int day = cob % 100;
    cob /= 100;
    int month = cob % 100;
    int year = cob / 100;

    char buffer[12];
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u", year, month, day);
    return std::string(buffer);
}

std::string Timestamp::cob2string() const { return cob2string(cob); }

std::string Timestamp::to_string() const { return to_string(cob, time); }

std::string Timestamp::to_string(int cob, int time) {
    int day = cob % 100;
    cob /= 100;
    int month = cob % 100;
    int year = cob / 100;

    int second = time % 100;
    time /= 100;
    int minute = time % 100;
    int hour = time / 100;

    char buffer[20];
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u %02u:%02u:%02u", year, month, day, hour, minute, second);
    return string(buffer);
}

int Timestamp::intraday_minute(int hms) {
    hms /= 100;
    int minute = hms % 100;
    int hour = hms / 100;
    return 60 * hour + minute;
}

int Timestamp::intraday_minute2hms(int intradayMinute) {
    int minute = intradayMinute % 60;
    int hour = intradayMinute / 60;
    return hour * 10000 + minute * 100;
}

bool operator<(const Timestamp& left, const Timestamp& right) {
    return left.cob < right.cob || (left.cob == right.cob && left.time < right.time);
}
bool operator>(const Timestamp& left, const Timestamp& right) {
    return left.cob > right.cob || (left.cob == right.cob && left.time > right.time);
}
bool operator==(const Timestamp& left, const Timestamp& right) {
    return left.cob == right.cob && left.time == right.time;
}
bool operator<=(const Timestamp& left, const Timestamp& right) {
    return left.cob < right.cob || (left.cob == right.cob && left.time <= right.time);
}
bool operator>=(const Timestamp& left, const Timestamp& right) {
    return left.cob > right.cob || (left.cob == right.cob && left.time >= right.time);
}
bool operator!=(const Timestamp& left, const Timestamp& right) { return !(left == right); }

ostream& operator<<(ostream& os, const Timestamp& timestamp) {
    os << timestamp.cob << "." << std::setw(6) << std::setfill('0') << timestamp.time;
    return os;
}
}
