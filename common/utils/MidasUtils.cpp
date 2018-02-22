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

long get_tid() { return syscall(__NR_gettid); }

string get_user_id() {
    uid_t uid = geteuid();
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        return std::string(pw->pw_name);
    }
    return std::string("");
}

void set_cpu_affinity(pid_t pid, const std::string& affinityString) {
    if (affinityString.empty()) return;
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

void set_cpu_affinity(const std::string& affinityString) {
    set_cpu_affinity(static_cast<pid_t>(get_tid()), affinityString);
}

bool destroy_timer(int* timerfd) {
    struct itimerspec newt = {{0, 0}, {0, 0}};
    struct itimerspec oldt = {{0, 0}, {0, 0}};
    if (timerfd_settime(*timerfd, 0, &newt, &oldt) < 0) {
        MIDAS_LOG_ERROR("Failed to reset timer " << *timerfd << " :" << errno << " " << strerror(errno));
    }
    close(*timerfd);
    *timerfd = -1;
    return true;
}

bool create_timer(uint32_t intv_msecs, int* timerfd) {
    bool rc = true;

    int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0) {
        MIDAS_LOG_ERROR("Failed to make timerfd " << errno << " " << strerror(errno));
        rc = false;
    } else {
        *timerfd = fd;
        uint32_t s = intv_msecs / 1000;
        uint32_t m = intv_msecs - s * 1000;
        struct itimerspec newt = {{s, m * 1000000}, {s, m * 1000000}};
        if (timerfd_settime(*timerfd, 0, &newt, nullptr) < 0) {
            MIDAS_LOG_ERROR("Failed to arm timer" << errno << " " << strerror(errno));
            destroy_timer(timerfd);
            rc = false;
        }
    }
    return rc;
}

bool stop_timer(int* timerfd) {
    bool rc = true;

    struct itimerspec newt = {{0, 0}, {0, 0}};
    struct itimerspec oldt = {{0, 0}, {0, 0}};
    if (timerfd_settime(*timerfd, 0, &newt, &oldt) < 0) {
        MIDAS_LOG_ERROR("Failed to stop timer " << *timerfd << " :" << errno << " " << strerror(errno));

        rc = false;
    }

    return rc;
}

bool restart_timer(uint32_t intv_msecs, int* timerfd) {
    bool rc = true;
    uint32_t s = intv_msecs / 1000;
    uint32_t m = intv_msecs - s * 1000;
    struct itimerspec newt = {{s, m * 1000}, {s, m * 1000}};
    struct itimerspec oldt = {{0, 0}, {0, 0}};
    if (timerfd_settime(*timerfd, 0, &newt, &oldt) < 0) {
        MIDAS_LOG_ERROR("Failed to restart timer " << *timerfd << " :" << errno << " " << strerror(errno));
        destroy_timer(timerfd);
        rc = false;
    }
    return rc;
}
}

#endif
