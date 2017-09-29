#ifndef NODE1_PROCESS_H
#define NODE1_PROCESS_H

#include <ctp/ThostFtdcTraderApi.h>
#include "../model/CtpData.h"
#include "TradeManager.h"
#include "TradeSpi.h"
#include "net/channel/Channel.h"
#include "net/tcp/TcpReceiver.h"
#include "process/MidasProcessBase.h"

using namespace std;
using namespace midas;

class CtpProcess : public MidasProcessBase {
public:
    CtpData data;

    CThostFtdcTraderApi* traderApi{nullptr};
    TradeSpi* traderSpi{nullptr};
    TradeManager* manager;

    std::thread marketDataThread;
    std::thread tradeDataThread;

    bool cancelAdmin{false};
    std::mutex mutexAdmin;
    std::condition_variable cvAdmin;
    std::map<uint64_t, string> outAdmin;

public:
    CtpProcess() = delete;
    CtpProcess(int argc, char** argv);
    ~CtpProcess();

protected:
    void app_start() override;
    void app_stop() override;

private:
    void set_log_level(int argc, char** argv) const;
    bool configure();
    void init_admin();

    // ctp section
    void trade_thread();
    void market_thread();

    // admin section
    string admin_meters(const string& cmd, const TAdminCallbackArgs& args) const;
    string admin_request(const string& cmd, const TAdminCallbackArgs& args);
    string admin_query(const string& cmd, const TAdminCallbackArgs& args);
    string admin_get_async_result(const string& cmd, const TAdminCallbackArgs& args, const std::string& userId);
    string admin_clear_async_result(const string& cmd, const TAdminCallbackArgs& args, const std::string& userId);
    string admin_buy(const string& cmd, const TAdminCallbackArgs& args) const;
    string admin_sell(const string& cmd, const TAdminCallbackArgs& args) const;
    string admin_close(const string& cmd, const TAdminCallbackArgs& args) const;
};

#endif
