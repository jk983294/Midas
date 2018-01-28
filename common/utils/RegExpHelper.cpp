#include "RegExpHelper.h"

namespace midas {
bool is_int(const std::string& str) {
    static const std::regex e("[-+]?\\d+");
    return regex_match(str, e);
}

bool is_double(const std::string& str) {
    static const std::regex e("[-+]?\\d+\\.\\d+");
    return regex_match(str, e);
}
}
