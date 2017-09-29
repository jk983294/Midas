#ifndef MIDAS_BACKOFF_H
#define MIDAS_BACKOFF_H

#include <sched.h>
#include <stdint.h>

namespace midas {

static inline void machine_pause(int32_t delay) {
    for (int32_t i = 0; i < delay; ++i) {
        __asm__ __volatile__("pause;");
    }
}

// do exponential backoff
class BackOff {
public:
    /**
     * time delay, in unit of pause instructions
     */
    static const int32_t LOOPS_BEFORE_YIELD{8192};
    int32_t count{1};

public:
    BackOff() {}

    void pause() {
        if (count <= LOOPS_BEFORE_YIELD) {
            machine_pause(count);
            count *= 2;
        } else {
            sched_yield();
        }
    }

    bool bounded_pause() {
        if (count <= LOOPS_BEFORE_YIELD) {
            machine_pause(count);
            count *= 2;
            return true;
        } else {
            return false;
        }
    }

    void reset() { count = 1; }

private:
    BackOff(const BackOff& rhs);
};
}

#endif
