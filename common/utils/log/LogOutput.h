#ifndef COMMON_UTILS_LOG_OUTPUT_H
#define COMMON_UTILS_LOG_OUTPUT_H

#include <iostream>
#include "LogData.h"

using namespace std;

namespace midas {
class NullOutput {
public:
    template <class T>
    void write(const T& data) {}
    void flush() {}
};

enum StdioFlag { COUT, CERR, COUT_CERR };

template <StdioFlag = StdioFlag::COUT>
class StdioOutput {
public:
    template <class LogData>
    void write(const LogData& data) {
        cout << data.msg() << '\n';
    }

    void flush() {}
};

template <>
class StdioOutput<StdioFlag::CERR> {
public:
    template <class LogData>
    void write(const LogData& data) {
        cerr << data.msg() << '\n';
    }

    void flush() {}
};

template <>
class StdioOutput<StdioFlag::COUT_CERR> {
public:
    template <class LogData>
    void write(const LogData& data) {
        std::ostream& os = data.priority() > WARNING ? std::cerr : std::cout;
        os << data.msg() << '\n';
    }

    void flush() {}
};
}

#endif
