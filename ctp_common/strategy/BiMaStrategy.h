#ifndef MIDAS_BI_MA_STRATEGY_H
#define MIDAS_BI_MA_STRATEGY_H

#include "StrategyBase.h"
#include "utils/math/DescriptiveStatistics.h"

class BiMaStrategy : public StrategyBase {
public:
    int slowPeriod{60};
    int fastPeriod{10};
    vector<double> slowMa;
    vector<double> fastMa;
    DescriptiveStatistics dsSlow;
    DescriptiveStatistics dsFast;

public:
    BiMaStrategy(const Candles& candles_) : StrategyBase(candles_) {}

    void calculate(size_t index) override {
        dsSlow.add_value(candles.data[index].close);
        dsFast.add_value(candles.data[index].close);
        slowMa[index] = dsSlow.get_mean();
        fastMa[index] = dsFast.get_mean();

        if (index > 10) {
            if (fastMa[index] > slowMa[index] && fastMa[index - 1] < slowMa[index - 1]) {
                signals[index] = 1.0;
                decision = StrategyDecision::longSignal;
            } else if (fastMa[index] < slowMa[index] && fastMa[index - 1] > slowMa[index - 1]) {
                signals[index] = -1.0;
                decision = StrategyDecision::shortSignal;
            }
        }
    }

    void init() override {
        size_t size = candles.data.size();
        dsSlow.clear();
        dsSlow.set_window_size(slowPeriod);
        dsFast.clear();
        dsFast.set_window_size(fastPeriod);
        slowMa.resize(size);
        fastMa.resize(size);
        signals.resize(size);
        fill(signals.begin(), signals.end(), 0);
    }

    string get_csv_header() override { return "slow,fast"; }

    string get_csv_line(size_t index) override {
        ostringstream oss;
        oss << slowMa[index] << "," << fastMa[index];
        return oss.str();
    }
};

#endif
