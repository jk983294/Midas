#ifndef MIDAS_TRAIN_COMMON_H
#define MIDAS_TRAIN_COMMON_H

#include <memory>
#include <ostream>
#include <vector>
#include "model/CtpOrder.h"

enum TrainType { SingleInt, SingleDouble };

class BacktestResult {
public:
    long cnt;
    double parameter, dayPerformance, kellyAnnualizedPerformance, stdDev, kellyFraction;
    double sharpeRatio, holdingDays;

    std::vector<std::shared_ptr<CtpPosition>> transactions;

public:
    void init(PositionManager& manager);
};

ostream& operator<<(ostream& os, const BacktestResult& result);

#endif
