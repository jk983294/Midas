#include "CtpProcess.h"

CtpProcess::CtpProcess(int argc, char** argv) : MidasProcessBase(argc, argv) {
    data = make_shared<CtpData>();
    manager = make_shared<TradeManager>(data);
    init_admin();
}

CtpProcess::~CtpProcess() {
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
    traderSpi = make_shared<TradeSpi>(manager, data);
    traderApi->RegisterSpi(traderSpi.get());                      // 注册事件类
    traderApi->SubscribePublicTopic(THOST_TERT_QUICK);            // 注册公有流
    traderApi->SubscribePrivateTopic(THOST_TERT_QUICK);           // 注册私有流
    traderApi->RegisterFront((char*)(data->tradeFront.c_str()));  // connect
    traderApi->Init();
    traderApi->Join();
}

void CtpProcess::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }

    marketManager = make_shared<MarketManager<CtpDataConsumer>>(std::make_shared<CtpDataConsumer>(data));
    marketManager->start();
    tradeDataThread = std::thread([this] { trade_thread(); });
    sleep(5);

    manager->init_ctp();

    std::unique_lock<std::mutex> lk(data->ctpMutex);
    data->ctpCv.wait(lk, [this] { return data->state == CtpState::TradeInitFinished; });
    data->state = CtpState::MarketInit;

    data->init_all_instruments();
    load_historic_candle_data();
    marketManager->mdSpi->subscribe_all_instruments(data->instrumentInfo);
    sleep(1);
    data->state = CtpState::Running;
}

void CtpProcess::app_stop() {
    if (tradeDataThread.joinable()) {
        tradeDataThread.join();
    }
}

void CtpProcess::load_historic_candle_data() {
    unordered_map<string, vector<CandleData>> historicCandle15;
    DaoManager::instance().candleDao->get_all_candles(historicCandle15);

    for (auto& item : data->instruments) {
        if (historicCandle15.find(item.first) != historicCandle15.end()) {
            item.second->load_historic_candle(historicCandle15[item.first]);
        }
    }
    MIDAS_LOG_INFO("load_historic_candle_data finish");
}
