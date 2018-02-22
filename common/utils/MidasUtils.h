#ifndef MIDAS_UITLS_MIDASUTILS_H
#define MIDAS_UITLS_MIDASUTILS_H

#include "utils/convert/MidasNumberUtils.h"

using namespace std;

namespace midas {

long get_tid();

// example "1:3:7-11:13"
void set_cpu_affinity(pid_t pid, const std::string& affinityString);
void set_cpu_affinity(const std::string& affinityString);

string get_user_id();

bool destroy_timer(int* timerfd);
bool create_timer(uint32_t intv_msecs, int* timerfd);
bool stop_timer(int* timerfd);
bool restart_timer(uint32_t intv_msecs, int* timerfd);

/**
 * hint for branch prediction
 */
inline bool is_likely_hint(bool exp) {
#ifdef __GNUC__
    return __builtin_expect(exp, true);
#else
    return exp;
#endif
}

inline bool is_unlikely_hint(bool exp) {
#ifdef __GNUC__
    return __builtin_expect(exp, false);
#else
    return exp;
#endif
}
}

#endif
