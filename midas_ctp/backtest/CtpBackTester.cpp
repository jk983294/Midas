#include <utils/CollectionUtils.h>
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
    fake_instrument_info_from_product();
    link_instrument_info();

    std::shared_ptr<BacktestResult> result = calculate(StrategyType::TBiMaStrategy);
    //    MIDAS_LOG_INFO("test result:\n" << *result);
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

    auto loadedProducts = dataLoader.load_products(productCfgFile);
    data->products.swap(loadedProducts);
}

std::shared_ptr<BacktestResult> CtpBackTester::calculate(StrategyType type) {
    StrategyFactory::set_strategy(data->instruments, type);
    std::unique_ptr<Simulator> simulator = make_unique<Simulator>(data);
    return simulator->get_performance();
}

void CtpBackTester::fake_instrument_info_from_product() {
    for (const auto& item : data->instruments) {
        const CtpInstrument& instrument = *item.second;
        string productId = instrument.productName;
        if (midas::is_exists(data->products, productId)) {
            std::shared_ptr<CThostFtdcInstrumentField> field(new CThostFtdcInstrumentField);
            const CThostFtdcProductField& productField = *data->products[productId];
            field->VolumeMultiple = productField.VolumeMultiple;
            field->PriceTick = productField.PriceTick;
            field->MaxMarketOrderVolume = productField.MaxMarketOrderVolume;
            field->MinMarketOrderVolume = productField.MinMarketOrderVolume;
            field->MaxLimitOrderVolume = productField.MaxLimitOrderVolume;
            field->MinLimitOrderVolume = productField.MinLimitOrderVolume;
            field->LongMarginRatio = 0.1;
            field->ShortMarginRatio = 0.1;
            data->instrumentInfo.insert({instrument.id, std::move(field)});
        } else {
            MIDAS_LOG_ERROR("cannot fake instrument info for " << instrument.id);
        }
    }
}

void CtpBackTester::link_instrument_info() {
    for (auto& item : data->instruments) {
        CtpInstrument& instrument = *item.second;
        if (midas::is_exists(data->instrumentInfo, instrument.id)) {
            instrument.info = data->instrumentInfo[instrument.id];
        } else {
            MIDAS_LOG_ERROR("cannot find instrument info for " << instrument.id);
        }
    }
}

void CtpBackTester::app_stop() { MidasProcessBase::app_stop(); }
