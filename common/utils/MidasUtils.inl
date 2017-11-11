#ifndef MIDAS_UTILS_MIDAS_UTILS_INL
#define MIDAS_UTILS_MIDAS_UTILS_INL

#include <linux/unistd.h>
#include <pwd.h>
#include <sched.h>
#include <sys/types.h>
#include <boost/tokenizer.hpp>
#include <cstdlib>
#include <fstream>
#include <string>
#include "midas/MidasConfig.h"
#include "utils/log/Log.h"

namespace midas {

inline long get_tid() { return syscall(__NR_gettid); }

inline string get_user_id() {
    uid_t uid = geteuid();
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        return std::string(pw->pw_name);
    }
    return std::string("");
}

inline void set_cpu_affinity(pid_t pid, const std::string& affinityString) {
    cpu_set_t cpuMask;
    CPU_ZERO(&cpuMask);

    boost::char_separator<char> commaSep(",");
    boost::tokenizer<boost::char_separator<char> > commaToken(affinityString, commaSep);

    for (auto itr = commaToken.begin(); itr != commaToken.end(); ++itr) {
        std::string commaPart(*itr);

        if (commaPart.find('-') != std::string::npos) {
            boost::char_separator<char> rangeSep("-");
            boost::tokenizer<boost::char_separator<char> > rangeToken(commaPart, rangeSep);
            auto rangeItr = rangeToken.begin();
            std::string rangeStart = *rangeItr;
            ++rangeItr;
            assert(rangeItr != rangeToken.end());
            std::string rangeEnd = *rangeItr;

            for (int i = atoi(rangeStart.c_str()); i < atoi(rangeEnd.c_str()); ++i) {
                CPU_SET(i, &cpuMask);
                MIDAS_LOG_INFO("affinity: enable CPU# " << i);
            }
        } else {
            int i = atoi(commaPart.c_str());
            CPU_SET(i, &cpuMask);
            MIDAS_LOG_INFO("affinity: enable CPU# " << i);
        }
    }

    if (sched_setaffinity(pid, sizeof(cpuMask), &cpuMask) != 0) {
        MIDAS_LOG_ERROR("failed to set affinity on string" << affinityString << " error: " << strerror(errno)
                                                           << " error no: " << errno << " pid: " << pid);
    }
}
}

#endif
