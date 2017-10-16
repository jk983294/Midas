#include "CtpProcess.h"

CtpProcess::CtpProcess(int argc, char** argv) : MidasProcessBase(argc, argv) { init_admin(); }

CtpProcess::~CtpProcess() {
    cancelAdmin = true;
    cvAdmin.notify_all();
}

void CtpProcess::trade_thread() {
    MIDAS_LOG_INFO("start trade data thread " << data.tradeFlowPath);
    traderApi = CThostFtdcTraderApi::CreateFtdcTraderApi(data.tradeFlowPath.c_str());
    manager = new TradeManager(traderApi, &data);
    traderSpi = new TradeSpi(manager, &data);
    traderApi->RegisterSpi((CThostFtdcTraderSpi*)traderSpi);     // 注册事件类
    traderApi->SubscribePublicTopic(THOST_TERT_QUICK);           // 注册公有流
    traderApi->SubscribePrivateTopic(THOST_TERT_QUICK);          // 注册私有流
    traderApi->RegisterFront((char*)(data.tradeFront.c_str()));  // connect
    traderApi->Init();
    traderApi->Join();
}

void CtpProcess::market_thread() {}

void CtpProcess::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }

    const string port = Config::instance().get<string>("cmd.server_port", "8023");
    // marketDataThread = std::thread([this] { market_thread(); });
    tradeDataThread = std::thread([this] { trade_thread(); });
    sleep(5);
    manager->init_ctp();
}

void CtpProcess::app_stop() {
    if (tradeDataThread.joinable()) {
        tradeDataThread.join();
    }
}
