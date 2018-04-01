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
    midas::DescriptiveStatistics dsSlow;
    midas::DescriptiveStatistics dsFast;

public:
    BiMaStrategy(const CtpInstrument& instrument_, CandleScale scale) : StrategyBase(instrument_, scale) {}

    void calculate(int index) override {
        dsSlow.add_value(candles.data[index].close);
        dsFast.add_value(candles.data[index].close);
        slowMa[index] = dsSlow.get_mean();
        fastMa[index] = dsFast.get_mean();

        if (index > 10) {
            if (coolDownCount > 0) {
                --coolDownCount;
                set_no_signal(itr);
            } else if (state == StrategyState::HoldMoney) {
                if (fastMa[index] > slowMa[index] && fastMa[index - 1] < slowMa[index - 1]) {
                    signals[index] = signalValueBuyThreshold;
                    decision = StrategyDecision::longSignal;
                    state = StrategyState::HoldLongPosition;
                } else if (fastMa[index] < slowMa[index] && fastMa[index - 1] > slowMa[index - 1]) {
                    signals[itr] = signalValueSellThreshold;
                    decision = StrategyDecision::shortSignal;
                    state = StrategyState::HoldShortPosition;
                } else {
                    set_no_signal(itr);
                }
            } else if (state == StrategyState::HoldLongPosition) {
                if (fastMa[index] < slowMa[index] && fastMa[index - 1] > slowMa[index - 1]) {
                    signals[itr] = signalValueSellThreshold;
                    decision = StrategyDecision::clearLongPositionSignal;
                    state = StrategyState::HoldMoney;
                    coolDownCount = 20;
                } else {
                    set_no_signal(itr);
                }
            } else if (state == StrategyState::HoldShortPosition) {
                if (fastMa[index] > slowMa[index] && fastMa[index - 1] < slowMa[index - 1]) {
                    signals[index] = signalValueBuyThreshold;
                    decision = StrategyDecision::clearShortPositionSignal;
                    state = StrategyState::HoldMoney;
                    coolDownCount = 20;
                } else {
                    set_no_signal(itr);
                }
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

    string get_csv_line(int index) override {
        ostringstream oss;
        oss << slowMa[index] << "," << fastMa[index];
        return oss.str();
    }
};

#endif
