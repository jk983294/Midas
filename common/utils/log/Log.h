#ifndef COMMON_UTILS_LOG_LOG_H
#define COMMON_UTILS_LOG_LOG_H

#include "LogData.h"
#include "LogWriterConcrete.h"
#include "midas/Lock.h"
#include "midas/Singleton.h"

namespace midas {
class Log {
public:
    typedef midas::LockType::mutex mutex_type;
    typedef midas::LockType::scoped_lock scoped_lock_type;
    LogWriterPtr writer_m;
    std::atomic<LogPriority> priority_m;
    mutex_type mutex_m;

public:
    ~Log() {}
    static Log& instance() { return Singleton<Log>::instance(); }

    friend class midas::Singleton<Log>;

    LogWriterPtr get_writer() {
        scoped_lock_type lock(mutex_m);
        return writer_m;
    }

    LogWriterPtr set_writer(LogWriterPtr writer) {
        scoped_lock_type lock(mutex_m);
        LogWriterPtr old = writer_m;
        writer_m = writer;
        return old;
    }

    inline bool can_log(LogPriority priority) { return priority >= priority_m; }

    inline void log(LogPriority priority, const char* file, int line, const LogMsg& msg) {
        writer_m->write_log(priority, file, line, msg.str());
    }

    inline std::string type_info() {
        std::ostringstream o;
        o << "Log info: lock type " << typeid(midas::LockType).name() << ", writer type "
          << typeid(*(writer_m.get())).name();
        return o.str();
    }

private:
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    Log() : writer_m(new StdioLogWriter), priority_m(INFO) {}
};
}

#define PEROFRM_LOG(priority, file, line, str)                                        \
    {                                                                                 \
        if (midas::Log::instance().can_log(priority)) {                               \
            midas::Log::instance().log(priority, file, line, midas::LogMsg() << str); \
        }                                                                             \
    }

static inline void PERFORM_LOG_PRINTF(midas::LogPriority priority, const char* file, int line, const char* format, ...)
    __attribute__((format(printf, 4, 5)));
void PERFORM_LOG_PRINTF(midas::LogPriority priority, const char* file, int line, const char* format, ...) {
    if (midas::Log::instance().can_log(priority)) {
        const size_t maxSize{1024};
        va_list vl;
        va_start(vl, format);
        char msg[maxSize] = {0};
        vsnprintf(msg, maxSize, format, vl);
        va_end(vl);
        std::string msg1(msg);
        midas::LogMsg m(msg1);
        midas::Log::instance().log(priority, file, line, m);
    }
}

#define MIDAS_LOG_SET_PRIORITY(priority) midas::Log::instance().priority_m = priority;
#define MIDAS_LOG_SET_WRITER(writer) midas::Log::instance().set_writer(midas::LogWriterPtr(writer));
#define MIDAS_LOG_INIT(priority, writer)                                \
    {                                                                   \
        midas::Log::instance().priority_m = priority;                   \
        midas::Log::instance().set_writer(midas::LogWriterPtr(writer)); \
    }

#define MIDAS_LOG_DEBUG(str) PEROFRM_LOG(midas::DEBUG, __FILE__, __LINE__, str)
#define MIDAS_LOG_INFO(str) PEROFRM_LOG(midas::INFO, __FILE__, __LINE__, str)
#define MIDAS_LOG_WARNING(str) PEROFRM_LOG(midas::WARNING, __FILE__, __LINE__, str)
#define MIDAS_LOG_ERROR(str) PEROFRM_LOG(midas::ERROR, __FILE__, __LINE__, str)

#define MIDAS_LOG_PRINTF_DEBUG(args...) PERFORM_LOG_PRINTF(midas::DEBUG, __FILE__, __LINE__, args)
#define MIDAS_LOG_PRINTF_INFO(args...) PERFORM_LOG_PRINTF(midas::INFO, __FILE__, __LINE__, args)
#define MIDAS_LOG_PRINTF_WARNING(args...) PERFORM_LOG_PRINTF(midas::WARNING, __FILE__, __LINE__, args)
#define MIDAS_LOG_PRINTF_ERROR(args...) PERFORM_LOG_PRINTF(midas::ERROR, __FILE__, __LINE__, args)

#define MIDAS_LOG_FLUSH() \
    { midas::Log::instance().get_writer()->flush_log(); }

#endif
