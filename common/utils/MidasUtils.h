#ifndef MIDAS_UITLS_MIDASUTILS_H
#define MIDAS_UITLS_MIDASUTILS_H

#include "utils/convert/MidasNumberUtils.h"

using namespace std;

namespace midas {

inline bool check_file_exists(const char* path);

// example "1:3:7-11:13"
inline void set_cpu_affinity(pid_t pid, const std::string& affinityString);

inline long get_tid();

inline string get_user_id();

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

#include "MidasUtils.inl"

#endif
