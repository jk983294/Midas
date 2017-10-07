#ifndef MIDAS_STRINGHELPER_H
#define MIDAS_STRINGHELPER_H

#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace midas {

inline string to_upper_case(const string& str) {
    const auto len = str.length();
    string ret(len, 0);
    for (std::remove_const<decltype(len)>::type pos = 0; pos < len; ++pos) {
        ret[pos] = static_cast<char>(std::toupper(str[pos]));
    }
    return ret;
}

inline string to_lower_case(const string& str) {
    const auto len = str.length();
    string ret(len, 0);
    for (std::remove_const<decltype(len)>::type pos = 0; pos < len; ++pos) {
        ret[pos] = static_cast<char>(std::tolower(str[pos]));
    }
    return ret;
}

inline void text_word_warp(const string& text, size_t maxLineSize, vector<string>& lines) {
    maxLineSize = std::max(maxLineSize, (size_t)16);
    size_t remaining = text.size();
    size_t textSize = text.size();
    size_t start{0};
    const char* pData = text.c_str();

    while (remaining > maxLineSize) {
        size_t end = start + maxLineSize;
        // find last whitespace
        while (!isspace(pData[end - 1] && end > start)) end--;
        // find last non whitespace
        while (isspace(pData[end - 1] && end > start)) end--;

        bool truncate = false;
        if (end == start) {
            end = start + maxLineSize - 1;
            truncate = true;
        }

        string line(text.substr(start, end - start));
        if (truncate) line += '-';

        lines.push_back(line);

        start = end;

        while (isspace(pData[start]) && start < textSize) ++start;

        remaining = textSize - start;
    }

    if (remaining > 0) {
        lines.push_back(text.substr(start, remaining));
    }
}

template <class T>
inline string string_join(const vector<T>& v, char delimiter = ' ') {
    ostringstream os;
    if (v.size() > 0) {
        os << v[0];
        for (size_t i = 1; i < v.size(); ++i) {
            os << delimiter << v[i];
        }
    }
    return os.str();
}

inline string get_bool_string(bool value) {
    if (value)
        return "true";
    else
        return "false";
}

inline string get_bool_string(int value) {
    if (value)
        return "true";
    else
        return "false";
}

inline bool is_localhost(const std::string& host) { return "localhost" == host || "127.0.0.1" == host; }

template <typename Object>
inline string object2str(Object& o) {
    ostringstream oss;
    oss << o;
    return oss.str();
}

struct MidasStringHashCompare {
    static size_t hash(const string& x) {
        size_t h = 0;
        for (const char* s = x.c_str(); *s; ++s) {
            h = (h * 17) ^ *s;
        }
        return h;
    }

    static bool equal(const string& x, const string& y) { return x == y; }
};
}

#endif
