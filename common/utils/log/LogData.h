#ifndef COMMON_UTILS_LOG_DATA_H
#define COMMON_UTILS_LOG_DATA_H

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <future>
#include <ostream>
#include <sstream>
#include <string>

// if not support, then disable it
#ifndef __GNUC__
#define __attribute__(x)
#endif

#define MIDAS_LOG_PRIORITY_STRING_ERROR "ERROR"
#define MIDAS_LOG_PRIORITY_STRING_WARNING "WARNING"
#define MIDAS_LOG_PRIORITY_STRING_INFO "INFO"
#define MIDAS_LOG_PRIORITY_STRING_DEBUG "DEBUG"

namespace midas {

enum LogPriority { DEBUG, INFO, WARNING, ERROR };
enum LogWriterPolicy { SYNC, ASYNC };

static inline const char* to_string(LogPriority priority) {
    switch (priority) {
        case DEBUG:
            return MIDAS_LOG_PRIORITY_STRING_DEBUG;
        case INFO:
            return MIDAS_LOG_PRIORITY_STRING_INFO;
        case WARNING:
            return MIDAS_LOG_PRIORITY_STRING_WARNING;
        case ERROR:
            return MIDAS_LOG_PRIORITY_STRING_ERROR;
        default:
            return "unknown";
    }
}

// if not found, return default INFO
static inline LogPriority from_string_log_priority(const char* str) {
    for (int i = DEBUG; i <= ERROR; i++) {
        if (strcmp(str, to_string(static_cast<LogPriority>(i))) == 0) return static_cast<LogPriority>(i);
    }
    return INFO;
}

class LogMsg {
    std::ostringstream oss_m;

public:
    typedef LogMsg& (*LogMsgFunctor)(LogMsg&);
    typedef std::basic_ostream<char, std::char_traits<char>> cout_t;
    typedef cout_t& (*StdFunctor)(cout_t&);

    ~LogMsg() {}
    LogMsg() {}

    LogMsg(const std::string&& msg) { oss_m << msg; }
    LogMsg(const std::string& msg) { oss_m << msg; }
    const std::string str() const { return oss_m.str(); }

    template <class T>
    LogMsg& operator<<(const T& t) {
        oss_m << t;
        return *this;
    }

    LogMsg& operator<<(LogMsgFunctor f) { return f(*this); }

    // allow msg << endl << alphabool
    LogMsg& operator<<(StdFunctor f) {
        f(oss_m);
        return *this;
    }

    LogMsg(const char* format, ...) __attribute__((format(printf, 2, 3))) {
        const size_t maxSize{1024};
        va_list vl;
        va_start(vl, format);
        char msg[maxSize];
        msg[0] = '\0';
        vsnprintf(msg, maxSize, format, vl);
        va_end(vl);
        oss_m << msg;
    }
};

class LogQueueItem {
public:
    LogPriority priority_m;
    std::string file_m;
    int line_m = 0;
    std::string msg_m;
    timespec logTime_m = {};
    size_t repeatCount_m = 0;
    int tid_m = 0;
    std::shared_ptr<std::promise<void>> flushPromise_m;

public:
    LogQueueItem(LogPriority priority, const char* file, int line, const std::string& msg, const timespec& logTime,
                 size_t count, int tid);

    explicit LogQueueItem(
        const std::shared_ptr<std::promise<void>>& flushPromise = std::shared_ptr<std::promise<void>>());

    void set(LogPriority priority, const char* file, int line, const std::string& msg, const timespec& logTime,
             size_t count, int tid);

    void update_duplicate(const timespec& logTime) {
        ++repeatCount_m;
        logTime_m = logTime;
    }

    bool equals(LogPriority priority, const char* file, int line, const std::string& msg, int tid) const;

    LogPriority priority() { return priority_m; }

    const std::string& msg() const { return msg_m; }
};
}

#include "LogData.inl"

#endif
