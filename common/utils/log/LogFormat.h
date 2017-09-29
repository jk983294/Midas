#ifndef COMMON_UTILS_LOG_FORMAT_H
#define COMMON_UTILS_LOG_FORMAT_H

#include <string>
#include "midas/Lock.h"
#include "utils/convert/TimeHelper.h"

using namespace std;

namespace midas {
class NullFormat {
public:
    template <class T>
    std::string format(const T& data) {
        return data.msg_m;
    }
};

class StdProcessFormat {
public:
    typedef midas::LockType::mutex mutex;
    typedef midas::LockType::scoped_lock scoped_lock;
    char buf[4086];
    string host;
    string procName;
    unsigned seqNo{0};
    pid_t pid{0};
    mutex mtx;

public:
    StdProcessFormat() {}
    StdProcessFormat(string procName_) { init(procName_); }

    void init(string procName_) {
        procName = procName_;
        host = "not found";
        if (gethostname(buf, sizeof(buf)) == 0) {
            host = buf;
        }

        pid = getpid();

        strncpy(buf, procName.c_str(), sizeof(buf));
        procName = basename(buf);  // get rid of path
    }

    template <class LogData>
    std::string format(const LogData& data) {
        ++seqNo;
        std::ostringstream os;
        os << timespec2string(data.logTime_m) << " " << host << " " << procName << "[" << pid << ":" << data.tid_m
           << "] " << seqNo << " " << to_string(data.priority_m) << " " << data.msg_m << " (" << data.file_m << ":"
           << data.line_m << ")";

        if (data.repeatCount_m) {
            os << " [repeated " << data.repeatCount_m << " times]";
        }
        return os.str();
    }
};
}

#endif
