#include "Node1Process.h"

bool Node1Process::configure() {
    static const char root[] = "node1";

    {
        auto str = get_cfg_value<string>(root, "test");
        MIDAS_LOG_INFO("get cfg value for test " << str);
    }

    if (adminPort.empty()) {
        MIDAS_LOG_INFO("must provide admin port!");
        return false;
    }
    if (dataPort.empty()) {
        MIDAS_LOG_INFO("must provide data port!");
        return false;
    }
    return true;
}
