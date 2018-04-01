#include "CtpOrder.h"

CtpOrder::CtpOrder(std::shared_ptr<CtpInstrument> instrument_, CtpDirection dir_, int size_, double price_)
    : instrument(instrument_), direction(dir_), requestSize(size_), requestPrice(price_) {}

void CtpOrder::mark2market() {
    const CThostFtdcInstrumentField& info = *instrument->info;
    double latestPrice = instrument->market_price();
    totalInstrumentValue = latestPrice * (requestSize - fulfilledSize) * info.VolumeMultiple;
    if (direction == CtpDirection::Short) {
        margin = totalInstrumentValue * info.ShortMarginRatio;
    } else {
        margin = totalInstrumentValue * info.LongMarginRatio;
    }
}

std::unique_ptr<CtpPosition> CtpOrder::fulfill(int tradedSize_, double tradedPrice_) {
    std::unique_ptr<CtpPosition> position(new CtpPosition(instrument, direction, tradedSize_, tradedPrice_));
    fulfilledSize += tradedSize_;
    return position;
}

CtpPosition::CtpPosition(std::shared_ptr<CtpInstrument> instrument_, CtpDirection dir_, int size_, double price_)
    : instrument(instrument_), direction(dir_), size(size_), openPrice(price_), previousPrice(price_) {
    mark2market();
    commissionFee = get_commission();
    profit = -get_commission();
    openTime = instrument->strategy->current_candle().timestamp;
}

void CtpPosition::mark2market() { mark2market(instrument->market_price()); }

void CtpPosition::mark2market(double latestPrice) {
    const CThostFtdcInstrumentField& info = *instrument->info;
    totalInstrumentValue = latestPrice * size * info.VolumeMultiple;
    deltaProfit = (latestPrice - previousPrice) * size * info.VolumeMultiple;
    if (direction == CtpDirection::Short) {
        deltaProfit = -deltaProfit;
        margin = totalInstrumentValue * info.ShortMarginRatio;
        deltaMargin = (latestPrice - previousPrice) * size * info.VolumeMultiple * info.ShortMarginRatio;
    } else {
        margin = totalInstrumentValue * info.LongMarginRatio;
        deltaMargin = (latestPrice - previousPrice) * size * info.VolumeMultiple * info.LongMarginRatio;
    }

    profit += deltaProfit;
    previousPrice = latestPrice;
}

void CtpPosition::set_close() {
    closed = true;
    deltaProfit = 0;
    deltaMargin = 0;
    double cost = get_commission();
    profit -= cost;
    commissionFee += cost;
    closeTime = instrument->strategy->current_candle().timestamp;
}

double CtpPosition::get_commission() {
    return std::max(commissionPerLot * size, totalInstrumentValue * commissionRate);
}

ostream& operator<<(ostream& os, const CtpPosition& position) {
    os << position.openTime << "," << position.closeTime << "," << position.openPrice << "," << position.previousPrice
       << "," << (position.direction == CtpDirection::Long ? "long" : "short") << "," << position.size << ","
       << position.profit << "," << position.commissionFee;
    return os;
}
