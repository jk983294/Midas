#include <trade/PositionManager.h>
#include "TrainCommon.h"

void BacktestResult::init(PositionManager& manager) {
    transactions.resize(manager.finished.size());
    copy(manager.finished.begin(), manager.finished.end(), transactions.begin());
    cnt = transactions.size();
}

ostream& operator<<(ostream& os, const BacktestResult& result) {
    os << "openTime,closeTime,openPrice,closePrice,direction,size,profit,commission\n";
    for (auto& position : result.transactions) {
        os << *position << endl;
    }
    return os;
}
