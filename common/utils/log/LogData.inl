#ifndef COMMON_UTILS_LOG_DATA_INL
#define COMMON_UTILS_LOG_DATA_INL

namespace midas {

inline LogQueueItem::LogQueueItem(LogPriority priority, const char* file, int line, const std::string& msg,
                                  const timespec& logTime, size_t count, int tid)
    : priority_m(priority),
      file_m(file),
      line_m(line),
      msg_m(msg),
      logTime_m(logTime),
      repeatCount_m(count),
      tid_m(tid) {}

inline LogQueueItem::LogQueueItem(const std::shared_ptr<std::promise<void>>& flushPromise)
    : flushPromise_m(flushPromise) {}

inline void LogQueueItem::set(LogPriority priority, const char* file, int line, const std::string& msg,
                              const timespec& logTime, size_t count, int tid) {
    priority_m = priority;
    file_m = file;
    line_m = line;
    msg_m = msg;
    logTime_m = logTime;
    repeatCount_m = count;
    tid_m = tid;
}

inline bool LogQueueItem::equals(LogPriority priority, const char* file, int line, const std::string& msg,
                                 int tid) const {
    return priority_m == priority && file_m == file && line_m == line && msg_m == msg && tid_m == tid;
}
}

#endif
