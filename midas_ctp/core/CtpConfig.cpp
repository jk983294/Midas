#include "CtpProcess.h"

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
    if (configValue != defaultValue) return configValue;

    configValue = Config::instance().get<T>(to_lower_case(path), defaultValue);
    if (configValue == defaultValue) {
        MIDAS_LOG_WARNING("config entry not found for " << key);
    }
    return configValue;
}

bool CtpProcess::configure() {
    static const char root[] = "ctp";

    data.brokerId = get_cfg_value<string>(root, "brokerId");
    data.investorId = get_cfg_value<string>(root, "investorId");
    data.password = get_cfg_value<string>(root, "password");
    data.tradeFront = get_cfg_value<string>(root, "tradeFront");
    data.marketFront = get_cfg_value<string>(root, "marketFront");
    data.dataDirectory = get_cfg_value<string>(root, "dataDirectory");
    data.tradeFlowPath = data.dataDirectory + "/tradeFlowPath/";
    data.marketFlowPath = data.dataDirectory + "/marketFlowPath/";

    if (!check_file_exists(data.tradeFlowPath.c_str())) {
        MIDAS_LOG_ERROR(data.tradeFlowPath << " trade flow path not exist!");
        return false;
    }

    if (!check_file_exists(data.marketFlowPath.c_str())) {
        MIDAS_LOG_ERROR(data.marketFlowPath << " market flow path not exist!");
        return false;
    }

    if (data.brokerId.empty()) {
        MIDAS_LOG_ERROR("empty brokerId!");
        return false;
    }
    if (data.investorId.empty()) {
        MIDAS_LOG_ERROR("empty investorId!");
        return false;
    }
    if (data.password.empty()) {
        MIDAS_LOG_ERROR("empty password!");
        return false;
    }
    if (data.tradeFront.empty()) {
        MIDAS_LOG_ERROR("empty tradeFront!");
        return false;
    }
    if (data.marketFront.empty()) {
        MIDAS_LOG_ERROR("empty marketFront!");
        return false;
    }

    MIDAS_LOG_INFO("using config" << endl
                                  << "brokerId: " << data.brokerId << endl
                                  << "investorId: " << data.investorId << endl
                                  << "tradeFront: " << data.tradeFront << endl
                                  << "marketFront: " << data.marketFront << endl);
    return true;
}
