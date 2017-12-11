#ifndef MIDAS_MIDAS_CONSTANTS_H
#define MIDAS_MIDAS_CONSTANTS_H

#include <string>
#include "midas/Singleton.h"

using namespace std;

namespace midas {
    namespace constants {
        const string AdminDelimiter{"}"};
        const string ConfigNullValue{"null"};
        const string DefaultChannelCfgPath = "midas.io.channel";
        const string mfDelimiter{";"};
    }
}

#endif  // MIDAS_MIDAS_CONSTANTS_H
