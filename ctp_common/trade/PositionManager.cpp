#include "PositionManager.h"

using namespace std;

void PositionManager::mark2market(const map<std::string, std::shared_ptr<CtpInstrument>>& instruments) {
    totalMargin = 0;
    totalProfit = 0;
    for (auto order : holdingOrders) {
        const CtpInstrument& instrument = *instruments.find(order->instrumentId)->second;
        double marketPrice = instrument.strategy->previous_candle().close;
        if (instrument.strategy->is_valid_time()) {
            marketPrice = instrument.strategy->current_candle().open;
        }
        order->mark2market(marketPrice, *instrument.info);
        totalMargin += order->margin;
        totalProfit += order->profit;
    }
}

void PositionManager::init4simulation() {
    capital = InitAsset;
    holdingOrders.clear();
    signalledOrders.clear();
    finishedOrders.clear();
}
