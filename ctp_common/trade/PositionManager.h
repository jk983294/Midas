#ifndef MIDAS_POSITION_MANAGER_H
#define MIDAS_POSITION_MANAGER_H

#include <model/CtpInstrument.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "model/CtpOrder.h"

class PositionManager {
public:
    std::unordered_map<int, std::shared_ptr<CtpOrder>> signalledOrders;
    std::vector<std::shared_ptr<CtpOrder>> holdingOrders;
    std::vector<std::shared_ptr<CtpOrder>> finishedOrders;
    int orderId;
    double capital;
    double totalProfit;
    double totalMargin;
    std::mutex mtx;

public:
    PositionManager() = default;

    void mark2market(const map<std::string, std::shared_ptr<CtpInstrument>>& instruments);

    void init4simulation();

    double get_cash() { return capital + totalProfit - totalMargin; }
};

#endif
