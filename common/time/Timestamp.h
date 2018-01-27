#ifndef MIDAS_TIMESTAMP_H
#define MIDAS_TIMESTAMP_H

namespace midas {
class Timestamp {
public:
    int cob{0};
    int time{0};

public:
    Timestamp() = default;
    Timestamp(int cob_, int time_) : cob(cob_), time(time_) {}

    static std::string time2string(int intradayTime);
    std::string time2string();

    static std::string cob2string(int cob);
    std::string cob2string();

    std::string to_string();
};

inline std::string Timestamp::time2string(int intradayTime) {
    int second = intradayTime % 100;
    intradayTime /= 100;
    int minute = intradayTime % 100;
    int hour = intradayTime / 100;

    char buffer[12];
    std::snprintf(buffer, sizeof buffer, "%02u:%02u:%02u", hour, minute, second);
    return buffer;
}

inline std::string Timestamp::time2string() { return time2string(time); }

inline std::string Timestamp::cob2string(int cob) {
    int day = cob % 100;
    cob /= 100;
    int month = cob % 100;
    int year = cob / 100;

    char buffer[12];
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u", year, month, day);
    return buffer;
}

inline std::string Timestamp::cob2string() { return cob2string(cob); }

inline std::string Timestamp::to_string() { return cob2string(cob) + " " + time2string(time); }
}

#endif
