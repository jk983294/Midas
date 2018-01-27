#ifndef MIDAS_SIMULATOR_H
#define MIDAS_SIMULATOR_H

#include "TrainCommon.h"
#include "model/CtpOrder.h"

class Simulator {
public:
    std::shared_ptr<CtpData> data;
    std::vector<CtpOrder> orders;

public:
    Simulator(std::shared_ptr<CtpData> data_): data(data_){}

    void apply(const StrategyParameter& parameter){

    }

    BacktestResult get_performance(){
        BacktestResult result;
        return result;
    }
};

#endif
