#ifndef COMMON_UTILS_TIME_HELPER_H
#define COMMON_UTILS_TIME_HELPER_H

#include <sys/time.h>
#include <boost/date_time.hpp>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include "MidasNumberUtils.h"

using std::string;

namespace midas {
static const uint64_t oneSecondNano{1000000000};

inline double timespec2double(const timespec& ts) {
    long usec = ts.tv_nsec / 1000;
    return static_cast<double>(ts.tv_sec) + (static_cast<double>(usec) / 1000000.0);
}

inline void double2timespec(double t, timespec& ts) {
    ts.tv_sec = static_cast<time_t>(t);
    double dt = (t - static_cast<double>(ts.tv_sec)) * 1000000.0;
    dt = floor(dt + 0.5);
    long usec = static_cast<long>(dt);
    ts.tv_nsec = usec * 1000;
}
inline uint64_t timespec2nanos(const timespec& ts) { return ts.tv_sec * oneSecondNano + ts.tv_nsec; }

inline void nanos2timespec(uint64_t t, timespec& ts) {
    ts.tv_sec = t / oneSecondNano;
    ts.tv_nsec = t % oneSecondNano;
}

inline int timespec_cmp(const timespec& t1, const timespec& t2) {
    if (t1.tv_sec < t2.tv_sec)
        return -1;
    else if (t1.tv_sec > t2.tv_sec)
        return 1;
    else if (t1.tv_nsec < t2.tv_nsec)
        return -1;
    else if (t1.tv_nsec > t2.tv_nsec)
        return 1;
    else
        return 0;
}

inline void timespec_diff(const timespec& t1, const timespec& t2, timespec diff) {
    const timespec* pStart = &t1;
    const timespec* pEnd = &t2;
    if (timespec_cmp(t1, t2) > 0) {
        pStart = &t2;
        pEnd = &t1;
    }

    long nsecDiff = pEnd->tv_nsec - pStart->tv_nsec;
    if (nsecDiff >= 0) {
        diff.tv_sec = pEnd->tv_sec - pStart->tv_sec;
        diff.tv_nsec = nsecDiff;
    } else {
        diff.tv_sec = pEnd->tv_sec - pStart->tv_sec - 1;
        diff.tv_nsec = oneSecondNano + nsecDiff;
    }
}

inline int64_t nanoSinceEpoch() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<int64_t>(ts.tv_sec) * oneSecondNano + ts.tv_nsec;
}

inline uint64_t nanoSinceEpochU() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * oneSecondNano + ts.tv_nsec;
}

inline uint64_t ntime() { return nanoSinceEpochU(); }

// YYYYMMDD format
inline uint32_t nano2date(uint64_t nano = 0) {
    if (nano == 0) {
        nano = nanoSinceEpoch();
    }
    time_t epoch = nano / oneSecondNano;
    struct tm tm;
    gmtime_r(&epoch, &tm);
    return (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
}

inline std::string time_t2string(const time_t ct) {
    if (!ct) return "N/A";
    struct tm tm;
    localtime_r(&ct, &tm);
    char buffer[21];
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u %02u:%02u:%02u", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buffer;
}
inline std::string ntime2string(uint64_t nano) {
    time_t epoch = nano / oneSecondNano;
    return time_t2string(epoch);
}
inline time_t time_t_from_ymdhms(int ymd, int hms) {
    struct tm tm;
    tm.tm_mday = ymd % 100;
    ymd /= 100;
    tm.tm_mon = ymd % 100 - 1;
    tm.tm_year = ymd / 100 - 1900;
    tm.tm_sec = hms % 100;
    hms /= 100;
    tm.tm_min = hms % 100;
    tm.tm_hour = hms / 100;
    return mktime(&tm);
}
inline uint64_t ntime_from_double(double ymdhms) {
    int date = static_cast<int>(ymdhms);
    int hms = static_cast<int>((ymdhms - date) * 1000000);
    return oneSecondNano * time_t_from_ymdhms(date, hms);
}

inline std::string timeval2string(const struct timeval& tv) {
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);
    char buffer[24];
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u %02u:%02u:%02u.%03u", tm.tm_year + 1900, tm.tm_mon + 1,
                  tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<uint16_t>(tv.tv_usec / 1000));
    return buffer;
}

inline std::string timespec2string(const timespec& ts) {
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
    char buffer[24];
    std::snprintf(buffer, sizeof buffer, "%4u-%02u-%02u %02u:%02u:%02u.%03u", tm.tm_year + 1900, tm.tm_mon + 1,
                  tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<uint16_t>(ts.tv_nsec / 1000000));
    return buffer;
}

inline bool string_2_second_intraday(const string& str, const string& format, unsigned& secs) {
    if (format == "HHMMSS" && str.length() >= 4) {
        const char* p = str.c_str();
        int hour = (10 * (p[0] - '0')) + (p[1] - '0');
        int minute = (10 * (p[2] - '0')) + (p[3] - '0');
        int second = 0;
        if (str.length() >= 6) second = (10 * (p[0] - '0')) + (p[1] - '0');

        if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
            secs = (hour * 60 * 60) + minute * 60 + second;
            return true;
        }
    }
    return false;
}

inline bool string_2_microsecond_intraday(const string& str, const string& format, unsigned& msecs) {
    if (format == "HHMMSS.MMM") {
        unsigned secs = 0;
        if (string_2_second_intraday(str, "HHMMSS", secs)) {
            msecs = secs * 1000;
            if (str.length() == 10) {
                const char* p = str.c_str();
                msecs += (100 * (p[7] - '0')) + (10 * (p[8] - '0')) + (p[9] - '0');
            }
            return true;
        }
    }
    return false;
}

inline int second_2_string_intraday(unsigned secs, char* buffer) {
    secs %= (24 * 60 * 60);
    size_t i = 0;
    int2str0pad(secs / (60 * 60), 2, &buffer[i]);
    i += 2;
    secs %= (60 * 60);
    int2str0pad(secs / 60, 2, &buffer[i]);
    i += 2;
    int2str0pad(secs % 60, 2, &buffer[i]);
    i += 2;
    return i;
}

inline bool second_2_string_intraday(unsigned secs, const string& format, string& buffer) {
    if (format == "HHMMSS") {
        char tmp[16];
        int n = second_2_string_intraday(secs, tmp);
        buffer = string(tmp, n);
        return true;
    }
    return false;
}

inline int microsecond_2_string_intraday(unsigned microsecond, char* buffer) {
    int i = second_2_string_intraday(microsecond / 1000, buffer);
    buffer[i++] = '.';
    int2str0pad(microsecond % 1000, 3, &buffer[i]);
    return i + 3;
}

inline bool microsecond_2_string_intraday(unsigned microsecond, const string& format, string& buffer) {
    if (format == "HHMMSS") {
        return second_2_string_intraday(microsecond / 1000, format, buffer);
    } else if (format == "HHMMSS.MMM") {
        char tmp[16];
        int n = microsecond_2_string_intraday(microsecond, tmp);
        buffer = string(tmp, n);
        return true;
    }
    return false;
}

inline std::time_t ptime2time_t(const boost::posix_time::ptime& t) {
    static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    const boost::posix_time::time_duration::sec_type x((t - epoch).total_seconds());
    return x;
}

inline timeval now_timeval() {
    timeval tv;
    gettimeofday(&tv, 0);
    return tv;
}

inline std::string now_string() {
    time_t tNow = time(NULL);
    struct tm tm;
    localtime_r(&tNow, &tm);
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%4u%02u%02u.%02u%02u%02u", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buffer;
}
/**
 * like 23:59:59
 * @param str
 * @return
 */
inline int intraday_time_HMS(const char* str) {
    return (str[0] - '0') * 100000 + (str[1] - '0') * 10000 + (str[3] - '0') * 1000 + (str[4] - '0') * 100 +
           (str[6] - '0') * 10 + (str[7] - '0');
}
inline int intraday_time_HMS(const string& str) { return intraday_time_HMS(str.c_str()); }
/**
 * like 23:59
 * @param str
 * @return
 */
inline int intraday_time_HM(const char* str) {
    return (str[0] - '0') * 1000 + (str[1] - '0') * 100 + (str[3] - '0') * 10 + (str[4] - '0');
}
inline int intraday_time_HM(const string& str) { return intraday_time_HM(str.c_str()); }
/**
 * like 20171112
 * @param str
 * @return
 */
inline int cob(const char* str) {
    return (str[0] - '0') * 10000000 + (str[1] - '0') * 1000000 + (str[2] - '0') * 100000 + (str[3] - '0') * 10000 +
           (str[4] - '0') * 1000 + (str[5] - '0') * 100 + (str[6] - '0') * 10 + (str[7] - '0');
}
inline int cob(const string& str) { return cob(str.c_str()); }
}

#endif
