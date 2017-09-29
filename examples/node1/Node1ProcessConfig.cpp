#include "Node1Process.h"

/**
 * first check env value, then check config value
 * if still not found, change config path to lower case
 * if still not found ,then use default value and log warning
 */
template <typename T>
T get_cfg_value(const char* root, const char* key, const T& defaultValue = T()) {
    T envValue = Config::instance().getenv<T>(key, defaultValue);

    if (envValue != defaultValue) return envValue;

    auto path = string{root} + "." + key;
    T configValue = Config::instance().get<T>(path, defaultValue);
    if (envValue != defaultValue) return envValue;

    configValue = Config::instance().get<T>(to_lower_case(path), defaultValue);
    if (configValue == defaultValue) {
        MIDAS_LOG_WARNING("config entry not found for " << key);
    }
    return configValue;
}

bool Node1Process::configure() {
    static const char root[] = "node1";
    bool ret = true;

    {
        auto str = get_cfg_value<string>(root, "test");
        MIDAS_LOG_INFO("get cfg value for test " << str);
    }
    return ret;
}
