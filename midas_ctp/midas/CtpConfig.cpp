#include "CtpProcess.h"
#include "utils/FileUtils.h"

bool CtpProcess::configure() {
    {
        const string root{"ctp.session"};
        data->brokerId = get_cfg_value<string>(root, "brokerId");
        data->investorId = get_cfg_value<string>(root, "investorId");
        data->password = get_cfg_value<string>(root, "password");
        data->tradeFront = get_cfg_value<string>(root, "tradeFront");

        if (data->brokerId.empty()) {
            MIDAS_LOG_ERROR("empty brokerId!");
            return false;
        }
        if (data->investorId.empty()) {
            MIDAS_LOG_ERROR("empty investorId!");
            return false;
        }
        if (data->password.empty()) {
            MIDAS_LOG_ERROR("empty password!");
            return false;
        }
        if (data->tradeFront.empty()) {
            MIDAS_LOG_ERROR("empty tradeFront!");
            return false;
        }
    }

    {
        const string root{"ctp"};
        data->dataDirectory = get_cfg_value<string>(root, "dataDirectory");
        data->tradeFlowPath = data->dataDirectory + "/tradeFlowPath/";

        if (!check_file_exists(data->tradeFlowPath.c_str())) {
            MIDAS_LOG_ERROR(data->tradeFlowPath << " trade flow path not exist!");
            return false;
        }

        string tradingHourCfgPath = get_cfg_value<string>(root, "tradingHourCfgPath");
        if (!check_file_exists(tradingHourCfgPath.c_str())) {
            MIDAS_LOG_ERROR(tradingHourCfgPath << " Trading Hour Cfg not exist!");
            return false;
        } else {
            data->tradeStatusManager.load_trade_session(tradingHourCfgPath);
        }
    }

    {
        const string root{"ctp.mysql"};
        DaoManager::instance().account.ip = get_cfg_value<string>(root, "ip");
        DaoManager::instance().account.userName = get_cfg_value<string>(root, "userName");
        DaoManager::instance().account.password = get_cfg_value<string>(root, "password");
        if (!DaoManager::instance().test_connection()) {
            MIDAS_LOG_ERROR("database connection failed!");
            return false;
        }
    }
    MIDAS_LOG_INFO("using config" << '\n'
                                  << "brokerId: " << data->brokerId << '\n'
                                  << "investorId: " << data->investorId << '\n'
                                  << "tradeFront: " << data->tradeFront << '\n');
    return true;
}
