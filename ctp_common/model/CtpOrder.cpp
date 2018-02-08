#include "CtpOrder.h"

CtpOrder::CtpOrder(const std::string& instrumentId_, CtpOrderDirection dir_, int size_, double price_)
    : instrumentId(instrumentId_), direction(dir_), requestSize(size_), requestPrice(price_) {}

void CtpOrder::mark2market(double marketPrice, const CThostFtdcInstrumentField& info) {
    profit = (marketPrice - tradedPrice) * tradedSize * info.VolumeMultiple;
    totalInstrumentValue = marketPrice * tradedSize * info.VolumeMultiple;
    if (direction == CtpOrderDirection::Short) {
        profit = -profit;
        margin = totalInstrumentValue * info.ShortMarginRatio;
    } else {
        margin = totalInstrumentValue * info.LongMarginRatio;
    }
}

std::unique_ptr<CtpOrder> CtpOrder::fill(int tradedSize_, double tradedPrice_) {
    std::unique_ptr<CtpOrder> order(new CtpOrder(*this));
    order->tradedSize = tradedSize_;
    order->tradedPrice = tradedPrice_;
    return order;
}
