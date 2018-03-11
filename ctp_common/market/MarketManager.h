#ifndef MIDAS_MARKET_MANAGER_H
#define MIDAS_MARKET_MANAGER_H

#include <net/disruptor/Disruptor.h>
#include <utils/FileUtils.h>
#include "MdSpi.h"

using namespace std;
using namespace midas;

template <typename DataConsumer>
class MarketManager {
public:
    typedef midas::Disruptor<CtpMdSpi, DataConsumer, MktDataPayload, midas::ConsumerStages::OneStage> TMktDataDisruptor;
    typename DataConsumer::SharedPtr consumerPtr;
    typename TMktDataDisruptor::SharedPtr disruptorPtr;
    CThostFtdcMdApi* mdApi{nullptr};
    std::shared_ptr<CtpMdSpi> mdSpi;
    std::thread marketDataThread;
    string marketFront;
    string dataDirectory, marketFlowPath;

public:
    MarketManager(typename DataConsumer::SharedPtr consumerPtr_) : consumerPtr(consumerPtr_) {
        marketFront = get_cfg_value<string>("ctp.session", "marketFront");
        dataDirectory = get_cfg_value<string>("ctp", "dataDirectory");
        if (marketFront.empty()) {
            MIDAS_LOG_ERROR("empty marketFront!");
        }
        if (marketFront.empty()) {
            MIDAS_LOG_ERROR("empty dataDirectory!");
        }

        marketFlowPath = dataDirectory + "/marketFlowPath/";
        if (!midas::check_file_exists(marketFlowPath.c_str())) {
            MIDAS_LOG_ERROR(marketFlowPath << " market flow path not exist!");
        }

        MIDAS_LOG_INFO("using config for market manager {"
                       << "marketFlowPath: " << marketFlowPath << ", "
                       << "marketFront: " << marketFront << '}');
    }

    ~MarketManager() {
        if (marketDataThread.joinable()) marketDataThread.join();
    }

    void start() {
        mdApi = CThostFtdcMdApi::CreateFtdcMdApi(marketFlowPath.c_str());
        mdSpi = make_shared<CtpMdSpi>(mdApi);
        std::vector<CtpMdSpi::SharedPtr> producerStore;
        producerStore.push_back(mdSpi);
        disruptorPtr = boost::make_shared<TMktDataDisruptor>("mktdata_disruptor", "ctp.mktdata_disruptor",
                                                             producerStore, boost::ref(*consumerPtr));
        MIDAS_LOG_INFO("start market data thread " << marketFlowPath);
        mdApi->RegisterSpi(mdSpi.get());  // register event handler class
        marketDataThread = std::thread([this] { run(); });
    }

    void stop() {
        if (mdApi) {
            mdApi->RegisterSpi(nullptr);
            mdApi->Release();
            mdApi = nullptr;
        }
    }

    void run() {
        mdApi->RegisterFront((char*)(marketFront.c_str()));  // connect
        mdApi->Init();
        mdApi->Join();
    }

    void stats(ostream& oss) {
        if (disruptorPtr) disruptorPtr->stats(oss);
        if (consumerPtr) consumerPtr->stats(oss);
    }
};

#endif
