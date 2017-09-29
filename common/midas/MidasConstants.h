#ifndef MIDAS_MIDAS_CONSTANTS_H
#define MIDAS_MIDAS_CONSTANTS_H

#include <string>
#include "midas/Singleton.h"

using namespace std;

namespace midas {

class MidasConstants {
public:
    const string AdminDelimiter{"}"};
    const string ConfigNullValue{"null"};

public:
    ~MidasConstants() {}
    static MidasConstants& instance() { return Singleton<MidasConstants>::instance(); }
    friend class midas::Singleton<MidasConstants>;

private:
    MidasConstants(const MidasConstants&) = delete;
    MidasConstants& operator=(const MidasConstants&) = delete;
    MidasConstants() {}
};
}

#endif  // MIDAS_MIDAS_CONSTANTS_H
