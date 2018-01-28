#ifndef MIDAS_TRAIN_COMMON_H
#define MIDAS_TRAIN_COMMON_H

enum TrainType {
    SingleInt,
    SingleDouble
};

class BacktestResult {
public:
    long cnt;
    double parameter, dayPerformance, kellyAnnualizedPerformance, stdDev, kellyFraction;
    double sharpeRatio, holdingDays;
};

#endif
