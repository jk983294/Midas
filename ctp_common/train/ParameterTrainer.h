#ifndef MIDAS_PARAMETER_TRAINER_H
#define MIDAS_PARAMETER_TRAINER_H

#include <vector>
#include "TrainCommon.h"
#include "strategy/StrategyBase.h"
#include "utils/log/Log.h"
#include "Simulator.h"


class ParameterTrainer {
public:
    int singleIntStart, singleIntEnd, singleIntStep;
    double singleDoubleStart, singleDoubleEnd, singleDoubleStep;
    TrainType trainType;
    vector<BacktestResult> results;
    std::unique_ptr<Simulator> simulator;

public:
    ParameterTrainer(int start, int end, int step) {
        singleIntStart = start;
        singleIntEnd = end;
        singleIntStep = step;
        trainType = TrainType::SingleInt;
    }

    ParameterTrainer(double start, double end, double step) {
        singleDoubleStart = start;
        singleDoubleEnd = end;
        singleDoubleStep = step;
        trainType = TrainType::SingleDouble;
    }

    void process() {
        StrategyParameter parameter;
        if(trainType == TrainType::SingleInt){
            for (int i = singleIntStart; i <= singleIntEnd; i += singleIntStep) {
                parameter.singleInt = i;
                process(parameter, i);
            }
        } else if(trainType == TrainType::SingleDouble){
            for (double i = singleDoubleStart; i <= singleDoubleEnd; i += singleDoubleStep) {
                parameter.singleDouble = i;
                process(parameter, i);
            }
        }
    }

    void process(const StrategyParameter& parameter, double param) {
        MIDAS_LOG_INFO("start training with parameter " << param);
        simulator->apply(parameter);
        BacktestResult result = simulator->get_performance();
        result.parameter = param;
        results.push_back(result);
    }

    void init_simulator(std::shared_ptr<CtpData> data, const string& strategyName){
        simulator = make_unique<Simulator>(data);
    }

};

#endif
