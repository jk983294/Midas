#ifndef NODE1_PROCESS_H
#define NODE1_PROCESS_H

#include <ctp/ThostFtdcMdApi.h>
#include <ctp/ThostFtdcTraderApi.h>
#include <net/disruptor/Disruptor.h>
#include <memory>
#include "CtpDataConsumer.h"
#include "MdSpi.h"
#include "TradeManager.h"
#include "TradeSpi.h"
#include "experiment/CtpDataLogConsumer.h"
#include "model/CtpData.h"
#include "net/channel/Channel.h"
#include "net/tcp/TcpReceiver.h"
#include "process/MidasProcessBase.h"

using namespace std;
using namespace midas;

class CtpProcess : public MidasProcessBase {
public:
    //    typedef CtpDataConsumer DataConsumer;
    typedef CtpDataLogConsumer DataConsumer;
    typedef midas::Disruptor<CtpMdSpi, DataConsumer, MktDataPayload, midas::ConsumerStages::OneStage> TMktDataDisruptor;

    std::shared_ptr<CtpData> data;

    CThostFtdcTraderApi* traderApi{nullptr};
    std::shared_ptr<TradeSpi> traderSpi;
    CThostFtdcMdApi* mdApi{nullptr};
    std::shared_ptr<CtpMdSpi> mdSpi;
    std::shared_ptr<TradeManager> manager;

    std::thread marketDataThread;
    std::thread tradeDataThread;
    typename DataConsumer::SharedPtr consumerPtr;
    typename TMktDataDisruptor::SharedPtr disruptorPtr;

public:
    CtpProcess() = delete;
    CtpProcess(int argc, char** argv);
    ~CtpProcess();

protected:
    void app_start() override;
    void app_stop() override;

private:
    bool configure();
    void init_admin();

    // ctp section
    void trade_thread();
    void market_thread();

private:
    // admin section
    string admin_meters(const string& cmd, const TAdminCallbackArgs& args) const;

    /**
     * send request to server, because its async nature, this interface won't get back any answer
     * use admin_query to get the real answer after a few seconds to let server respond
     * use admin_dump to dump real answer
     */
    string admin_request(const string& cmd, const TAdminCallbackArgs& args);
    string admin_query(const string& cmd, const TAdminCallbackArgs& args);
    string admin_dump(const string& cmd, const TAdminCallbackArgs& args);

    string admin_get_async_result(const string& cmd, const TAdminCallbackArgs& args, const std::string& userId);
    string admin_clear_async_result(const string& cmd, const TAdminCallbackArgs& args, const std::string& userId);
    string admin_buy(const string& cmd, const TAdminCallbackArgs& args) const;
    string admin_sell(const string& cmd, const TAdminCallbackArgs& args) const;
    string admin_close(const string& cmd, const TAdminCallbackArgs& args) const;

    string flush(const string& cmd, const TAdminCallbackArgs& args);
};

#endif
