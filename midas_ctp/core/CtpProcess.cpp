#include "CtpProcess.h"

CtpProcess::CtpProcess(int argc, char** argv) : MidasProcessBase(argc, argv) {
    data = shared_ptr<CtpData>(new CtpData());
    manager = shared_ptr<TradeManager>(new TradeManager(data));
    init_admin();
}

CtpProcess::~CtpProcess() {
    if (mdApi) {
        mdApi->RegisterSpi(nullptr);
        mdApi->Release();
        mdApi = nullptr;
    }

    if (traderApi) {
        traderApi->RegisterSpi(nullptr);
        traderApi->Release();
        traderApi = nullptr;
    }
}

void CtpProcess::trade_thread() {
    MIDAS_LOG_INFO("start trade data thread " << data->tradeFlowPath);
    traderApi = CThostFtdcTraderApi::CreateFtdcTraderApi(data->tradeFlowPath.c_str());
    manager->register_trader_api(traderApi);
    traderSpi = shared_ptr<TradeSpi>(new TradeSpi(manager, data));
    traderApi->RegisterSpi(traderSpi.get());                      // 注册事件类
    traderApi->SubscribePublicTopic(THOST_TERT_QUICK);            // 注册公有流
    traderApi->SubscribePrivateTopic(THOST_TERT_QUICK);           // 注册私有流
    traderApi->RegisterFront((char*)(data->tradeFront.c_str()));  // connect
    traderApi->Init();
    traderApi->Join();
}

void CtpProcess::market_thread() {
    MIDAS_LOG_INFO("start market data thread " << data->marketFlowPath);
    mdApi = CThostFtdcMdApi::CreateFtdcMdApi(data->marketFlowPath.c_str());
    manager->register_md_api(mdApi);
    mdApi->RegisterSpi(mdSpi.get());                           // register event handler class
    mdApi->RegisterFront((char*)(data->marketFront.c_str()));  // connect
    mdApi->Init();
    mdApi->Join();
}

void CtpProcess::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }

    mdSpi = shared_ptr<CtpMdSpi>(new CtpMdSpi(manager, data));

    std::vector<CtpMdSpi::SharedPtr> producerStore;
    producerStore.push_back(mdSpi);
    consumerPtr = std::make_shared<DataConsumer>(data);
    disruptorPtr = boost::make_shared<TMktDataDisruptor>("mktdata_disruptor", "ctp.mktdata_disruptor", producerStore,
                                                         boost::ref(*consumerPtr));

    marketDataThread = std::thread([this] { market_thread(); });
    tradeDataThread = std::thread([this] { trade_thread(); });
    sleep(5);

    manager->init_ctp();

    std::unique_lock<std::mutex> lk(data->ctpMutex);
    data->ctpCv.wait(lk, [this] { return data->state == TradeInitFinished; });
    data->state = MarketInit;

    data->books.init_all_instruments(data->instruments);
    manager->subscribe_all_instruments();
    sleep(1);
    data->state = Running;
}

void CtpProcess::app_stop() {
    if (marketDataThread.joinable()) {
        marketDataThread.join();
    }
    if (tradeDataThread.joinable()) {
        tradeDataThread.join();
    }
}
