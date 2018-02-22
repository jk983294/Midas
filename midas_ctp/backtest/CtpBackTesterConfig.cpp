#include "CtpBackTester.h"
#include "utils/FileUtils.h"

bool CtpBackTester::configure() {
    static const char root[] = "backtest";

    dataDirectory = get_cfg_value<string>(root, "dataDirectory", "");
    productCfgFile = get_cfg_value<string>(root, "productCfgPath", "");
    candleScale = CandleScale(get_cfg_value<int>(root, "candleScale", 1));

    if (dataDirectory.empty()) {
        MIDAS_LOG_WARNING("data path not provided.");
    } else if (!check_file_exists(dataDirectory.c_str())) {
        MIDAS_LOG_ERROR(dataDirectory << " data path not exist!");
        return false;
    }

    if (productCfgFile.empty()) {
        MIDAS_LOG_ERROR("product cfg file not provided.");
        return false;
    } else if (!check_file_exists(productCfgFile.c_str())) {
        MIDAS_LOG_ERROR(productCfgFile << " product cfg file not exist!");
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
