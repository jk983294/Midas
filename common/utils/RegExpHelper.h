#ifndef MIDAS_REG_EXP_HELPER_H
#define MIDAS_REG_EXP_HELPER_H

#include <regex>
#include <string>

namespace midas {

inline bool is_int(const std::string& str) {
    static const std::regex e("[-+]?\\d+");
    return regex_match(str, e);
}

inline bool is_double(const std::string& str) {
    static const std::regex e("[-+]?\\d+\\.\\d+");
    return regex_match(str, e);
}
}

#endif
