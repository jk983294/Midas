#ifndef MIDAS_CTP_ORDER_H
#define MIDAS_CTP_ORDER_H

enum CtpOrderDirection { Long, Short };
enum CtpOrderType { Market, Limit };

class CtpOrder {
public:
    CtpOrderDirection direction{CtpOrderDirection::Long};
    CtpOrderType type{CtpOrderType::Limit};
    int size{0};
    int tradedSize{0};
    double price{0};
    double tradedPrice{0};

public:
    CtpOrder() = default;
    CtpOrder(CtpOrderDirection dir_, int size_, double price_);
};

#endif
