#include "CtpBackTester.h"
#include "utils/FileUtils.h"

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

bool CtpBackTester::configure() {
    static const char root[] = "backtest";

    dataDirectory = get_cfg_value<string>(root, "dataDirectory", "");
    candleScale = CandleScale(get_cfg_value<int>(root, "candleScale", 1));

    if (dataDirectory.empty()) {
        MIDAS_LOG_WARNING("data path not provided.");
    } else if (!check_file_exists(dataDirectory.c_str())) {
        MIDAS_LOG_ERROR(dataDirectory << " data path not exist!");
        return false;
    }

    string tradingHourCfgPath = get_cfg_value<string>(root, "tradingHourCfgPath");
    if (!check_file_exists(tradingHourCfgPath.c_str())) {
        MIDAS_LOG_ERROR(tradingHourCfgPath << " Trading Hour Cfg not exist!");
        return false;
    } else {
        data->tradeStatusManager.load_trade_session(tradingHourCfgPath);
    }

    MIDAS_LOG_INFO("using config" << '\n'
                                  << "dataDirectory: " << dataDirectory << '\n'
                                  << "candleScale: " << candleScale << '\n');
    return true;
}
