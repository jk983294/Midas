#ifndef COMMON_UTILS_LOG_WRITER_INL
#define COMMON_UTILS_LOG_WRITER_INL

#include <boost/bind.hpp>
#include "LogData.h"

namespace midas {

template <class Output, class Format, LogWriterPolicy LWP>
inline void LogWriterT<Output, Format, LWP>::write_log(LogPriority priority, const char* file, int line,
                                                       const std::string& msg) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    int tid = syscall(__NR_gettid);

    scoped_lock lock(mutex_m);
    if (lastItem_m.equals(priority, file, line, msg, tid)) {
        lastItem_m.update_duplicate(now);  // if dup, then no write util last dup
        return;
    }

    if (lastItem_m.repeatCount_m) {
        // call output.write
        write(LogQueueItem(lastItem_m.priority_m, lastItem_m.file_m, lastItem_m.line_m, format(lastItem_m),
                           lastItem_m.logTime_m, lastItem_m.repeatCount_m, lastItem_m.tid_m));
    }
    lastItem_m.set(priority, file, line, msg, now, 0, tid);
    write(LogQueueItem(lastItem_m.priority_m, lastItem_m.file_m, lastItem_m.line_m, format(lastItem_m),
                       lastItem_m.logTime_m, lastItem_m.repeatCount_m, lastItem_m.tid_m));
}

// specialization async log writer
template <class Output, class Format>
inline void LogWriterT<Output, Format, LogWriterPolicy::ASYNC>::write_log(LogPriority priority, const char* file,
                                                                          int line, const std::string& msg) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    int tid = syscall(__NR_gettid);

    scoped_lock lock(mutex_m);
    if (lastItem_m.equals(priority, file, line, msg, tid)) {
        lastItem_m.update_duplicate(now);  // if dup, then no write util last dup
        return;
    }

    if (lastItem_m.repeatCount_m) {
        dropped_m += q_m.try_push(LogQueueItem(lastItem_m.priority_m, lastItem_m.file_m.c_str(), lastItem_m.line_m,
                                               lastItem_m.msg_m, lastItem_m.logTime_m, lastItem_m.repeatCount_m,
                                               lastItem_m.tid_m));
    }
    lastItem_m.set(priority, file, line, msg, now, 0, tid);
    dropped_m +=
        q_m.try_push(LogQueueItem(lastItem_m.priority_m, lastItem_m.file_m.c_str(), lastItem_m.line_m, lastItem_m.msg_m,
                                  lastItem_m.logTime_m, lastItem_m.repeatCount_m, lastItem_m.tid_m));
}

template <class Output, class Format>
inline void LogWriterT<Output, Format, LogWriterPolicy::ASYNC>::flush_log() {
    scoped_lock lock(mutex_m);
    std::shared_ptr<std::promise<void>> flushPromise(std::make_shared<std::promise<void>>());
    auto future = flushPromise->get_future();
    q_m.try_push(LogQueueItem(flushPromise));
    future.wait_for(flushTimeout_m);
}

template <class Output, class Format>
inline void LogWriterT<Output, Format, LogWriterPolicy::ASYNC>::on_batch_timeout(
    const boost::system::error_code& error) {
    if (!error) {
        process_queue(batchSize_m);
    } else {
        std::clog << "error in LogWriterT<Output, Format, ASYNC>::on_batch_timeout " << error << '\n';
    }

    if (isRunning_m) {
        batchTimer_m.expires_from_now(boost::posix_time::milliseconds(batchInterval_m));
        batchTimer_m.async_wait(boost::bind(&LogWriterT::on_batch_timeout, this, boost::asio::placeholders::error));
    }
}

template <class Output, class Format>
inline void LogWriterT<Output, Format, LogWriterPolicy::ASYNC>::process_queue(size_t size) {
    for (size_t i = 0; i < size; i++) {
        LogQueueItem item;
        if (q_m.try_pop(item)) {
            std::shared_ptr<std::promise<void>>& flushPromise = item.flushPromise_m;
            if (flushPromise) {
                flush();
                flushPromise->set_value();
            } else {
                item.msg_m = Format::format(item);
                write(item);
            }
        } else {
            break;
        }
    }
}

template <class Output, class Format>
inline void LogWriterT<Output, Format, LogWriterPolicy::ASYNC>::service() {
    try {
        keepAliveTimer_m.expires_from_now(boost::posix_time::pos_infin);
        keepAliveTimer_m.async_wait(boost::bind(&LogWriterT::keep_alive, this, boost::asio::placeholders::error));

        batchTimer_m.expires_from_now(boost::posix_time::milliseconds(batchInterval_m));
        batchTimer_m.async_wait(boost::bind(&LogWriterT::on_batch_timeout, this, boost::asio::placeholders::error));

        while (isRunning_m) {
            iosvc_m.run();
        }
    } catch (...) {
        std::clog << "LogWriterT unhandled exception in service loop " << '\n';
    }
}
}

#endif
