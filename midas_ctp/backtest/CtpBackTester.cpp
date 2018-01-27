#include "CtpBackTester.h"
#include "helper/CtpHelper.h"
#include "helper/DataLoader.h"
#include "strategy/StrategyFactory.h"

CtpBackTester::CtpBackTester(int argc, char** argv) : MidasProcessBase(argc, argv) {
    data = make_shared<CtpData>();
    init_admin();
}

CtpBackTester::~CtpBackTester() {}

void CtpBackTester::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }

    if (dataDirectory.empty()) return;

    load_test_data(dataDirectory);

    calculate(StrategyType::TBiMaStrategy);
}

void CtpBackTester::load_test_data(const string& dataPath) {
    DataLoader dataLoader;
    dataLoader.load(dataPath);

    for (auto& item : dataLoader.instrument2candle) {
        const TradeSessions& pts = data->tradeStatusManager.get_session(item.first);
        std::shared_ptr<CtpInstrument> instrument = make_shared<CtpInstrument>(item.first, pts);
        instrument->load_historic_candle(item.second, CandleScale::Minute1);
        set_master_contract(*instrument);
        data->instruments.insert({item.first, instrument});
    }
}

BacktestResult CtpBackTester::calculate(StrategyType type) {
    StrategyFactory::set_strategy(data->instruments, type);
    std::unique_ptr<Simulator> simulator = make_unique<Simulator>(data);
    BacktestResult result;
    return result;
}

void CtpBackTester::app_stop() { MidasProcessBase::app_stop(); }
