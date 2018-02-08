#ifndef MIDAS_CTP_ORDER_H
#define MIDAS_CTP_ORDER_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <memory>
#include <string>
#include "model/CtpDef.h"

class CtpOrder {
public:
    std::string instrumentId;
    CtpOrderDirection direction{CtpOrderDirection::Long};
    CtpOrderType type{CtpOrderType::Limit};
    int id{0};
    int requestSize{0};
    int tradedSize{0};
    double requestPrice{0};
    double tradedPrice{0};
    double profit{0};
    double margin{0};
    double totalInstrumentValue{0};

public:
    CtpOrder() = default;
    CtpOrder(const std::string& instrumentId_, CtpOrderDirection dir_, int size_, double price_);

    void mark2market(double marketPrice, const CThostFtdcInstrumentField& info);

    /**
     * used to move signalledOrders to holdingOrders
     */
    std::unique_ptr<CtpOrder> fill(int tradedSize_, double tradedPrice_);
};

#endif
