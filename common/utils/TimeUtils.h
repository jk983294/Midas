#ifndef MIDAS_TIME_UTILS_H
#define MIDAS_TIME_UTILS_H

#include <cstdint>
#include <ctime>

namespace midas {

/**
 * used to measure time elapsed for some operations
 */
class Timer {
public:
    void start() { now(m_startTime); }

    void stop() { now(m_stopTime); }

    /**
     * calculate time elapsed
     * @return the elapsed time in nano secs
     */
    uint64_t elapsed() const {
        static const uint64_t nanosInSec = 1LL * 1000LL * 1000LL * 1000LL;
        return (((m_stopTime.tv_sec * nanosInSec) + m_stopTime.tv_nsec) -
                ((m_startTime.tv_sec * nanosInSec) + m_startTime.tv_nsec));
    }

private:
    void now(timespec& ts) { clock_gettime(CLOCK_MONOTONIC, &ts); }

    timespec m_startTime;
    timespec m_stopTime;
};
}

#endif
