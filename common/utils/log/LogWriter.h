#ifndef COMMON_UTILS_LOG_WRITER_H
#define COMMON_UTILS_LOG_WRITER_H

#include <sys/syscall.h>
#include <sys/time.h>
#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include <memory>
#include <string>
#include "LogData.h"
#include "midas/Lock.h"
#include "midas/MidasException.h"
#include "utils/convert/TimeHelper.h"

namespace midas {

class LogWriter {
public:
    virtual ~LogWriter() {}
    virtual void write_log(LogPriority priority, const char* file, int line, const std::string& msg) = 0;
    virtual void flush_log() = 0;
    std::string get_time_now() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return timeval2string(tv);
    }
};

typedef std::shared_ptr<LogWriter> LogWriterPtr;

template <class Output, class Format, LogWriterPolicy = SYNC>
class LogWriterT : public LogWriter, public Output, public Format {
public:
    typedef midas::LockType::mutex mutex;
    typedef midas::LockType::scoped_lock scoped_lock;
    typedef Output output_type;
    typedef Format format_type;
    using Output::write;
    using Output::flush;
    using Format::format;

protected:
    mutex mutex_m;
    LogQueueItem lastItem_m;

public:
    ~LogWriterT() {}
    void write_log(LogPriority priority, const char* file, int line, const std::string& msg) override final;
    void flush_log() override final { flush(); }
};

// specialization async log writer
template <class Output, class Format>
class LogWriterT<Output, Format, ASYNC> : public LogWriter, public Output, public Format {
public:
    typedef midas::LockType::mutex mutex;
    typedef midas::LockType::scoped_lock scoped_lock;
    typedef Output output_type;
    typedef Format format_type;
    using Output::write;
    using Output::flush;
    using Format::format;

    boost::asio::io_service iosvc_m;
    boost::asio::deadline_timer keepAliveTimer_m;
    boost::asio::deadline_timer batchTimer_m;
    tbb::concurrent_bounded_queue<LogQueueItem> q_m;
    std::atomic<size_t> dropped_m;
    std::atomic<size_t> batchSize_m;
    std::atomic<size_t> batchInterval_m;
    std::atomic<bool> isRunning_m;
    boost::thread thread_m;
    std::chrono::seconds flushTimeout_m;

protected:
    mutex mutex_m;
    LogQueueItem lastItem_m;

public:
    explicit LogWriterT(const std::chrono::seconds& timeout = std::chrono::seconds(5))
        : keepAliveTimer_m(iosvc_m), batchTimer_m(iosvc_m), flushTimeout_m(timeout) {
        isRunning_m = false;
        batchSize_m = 1024;
        batchInterval_m = 250;  // ms
        start();
    };
    ~LogWriterT() { stop(); }
    void start() {
        scoped_lock lock(mutex_m);
        if (!thread_m.joinable()) {
            isRunning_m = true;
            thread_m = boost::thread(&LogWriterT::service, this);
        }
    }
    void stop() {
        scoped_lock lock(mutex_m);
        if (isRunning_m) {
            isRunning_m = false;
            iosvc_m.stop();
            thread_m.join();
            process_queue(q_m.size());
        }
    }
    void write_log(LogPriority priority, const char* file, int line, const std::string& msg) override final;
    void flush_log() override final;

    void keep_alive(const boost::system::error_code& error) {
        if (error) {
            std::clog << "LogWriterT keep alive timer error " << error << '\n';
        }
    }
    void on_batch_timeout(const boost::system::error_code& error);
    void process_queue(size_t size);
    void service();
};
}

#include "utils/log/LogWriter.inl"

#endif
