#ifndef MIDAS_CTP_ORDER_H
#define MIDAS_CTP_ORDER_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <memory>
#include <ostream>
#include <string>
#include "CtpInstrument.h"
#include "model/CtpDef.h"

class CtpPosition;

class CtpOrder {
public:
    std::shared_ptr<CtpInstrument> instrument;
    CtpDirection direction{CtpDirection::Long};
    CtpOrderType type{CtpOrderType::Limit};
    int id{0};
    int requestSize{0};
    int fulfilledSize{0};
    double requestPrice{0};
    double margin{0};
    double totalInstrumentValue{0};

public:
    CtpOrder() = default;
    CtpOrder(std::shared_ptr<CtpInstrument> instrument_, CtpDirection dir_, int size_, double price_);

    void mark2market();

    /**
     * used to move signalledOrders to holdingOrders
     */
    std::unique_ptr<CtpPosition> fulfill(int tradedSize_, double tradedPrice_);
};

class CtpPosition {
public:
    std::shared_ptr<CtpInstrument> instrument;
    CtpDirection direction{CtpDirection::Long};
    bool closed{false};
    int size{0};
    double openPrice{0};
    double previousPrice{0};         // track latest price, this is also close price
    double deltaProfit{0};           // (latestPrice - previousPrice) * size * VolumeMultiple
    double profit{0};                // (latestPrice - tradedPrice) * size * VolumeMultiple
    double totalInstrumentValue{0};  // latestPrice * size * VolumeMultiple
    double margin{0};                // totalInstrumentValue * ShortMarginRatio
    double deltaMargin{0};           // (latestPrice - previousPrice) * size * VolumeMultiple * ShortMarginRatio
    double commissionFee{0};
    double totalAccountValue{0};
    midas::Timestamp openTime;
    midas::Timestamp closeTime;

public:
    CtpPosition(std::shared_ptr<CtpInstrument> instrument_, CtpDirection dir_, int size_, double price_);

    void mark2market();

    void mark2market(double latestPrice);

    void set_close();

    double get_commission();
};

ostream& operator<<(ostream& os, const CtpPosition& position);

#endif
