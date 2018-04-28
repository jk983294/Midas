#ifndef MIDAS_BI_MA_STRATEGY_H
#define MIDAS_BI_MA_STRATEGY_H

#include <sstream>
#include "StrategyBase.h"
#include "utils/math/DescriptiveStatistics.h"
#include "utils/math/MathHelper.h"

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

    void calculate(int index, double marketPrice) override {
        if (index == 0) return;
        dsSlow.add_value(candles.data[index - 1].close);
        dsFast.add_value(candles.data[index - 1].close);
        slowMa[index - 1] = dsSlow.get_mean();
        fastMa[index - 1] = dsFast.get_mean();

        if (index > 10) {
            if (coolDownCount > 0) {
                --coolDownCount;
                set_no_signal(itr);
            } else if (state == StrategyState::HoldMoney) {
                if (midas::is_up_cross(fastMa, slowMa, index - 1)) {
                    signals[index] = signalValueBuyThreshold;
                    decision = StrategyDecision::longSignal;
                    state = StrategyState::HoldLongPosition;
                } else if (midas::is_down_cross(fastMa, slowMa, index - 1)) {
                    signals[itr] = signalValueSellThreshold;
                    decision = StrategyDecision::shortSignal;
                    state = StrategyState::HoldShortPosition;
                } else {
                    set_no_signal(itr);
                }
            } else if (state == StrategyState::HoldLongPosition) {
                if (midas::is_down_cross(fastMa, slowMa, index - 1)) {
                    signals[itr] = signalClearLong;
                    decision = StrategyDecision::clearLongPositionSignal;
                    state = StrategyState::HoldMoney;
                    coolDownCount = 20;
                } else {
                    set_no_signal(itr);
                }
            } else if (state == StrategyState::HoldShortPosition) {
                if (midas::is_up_cross(fastMa, slowMa, index - 1)) {
                    signals[index] = signalClearShort;
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
        __init();
        size_t size = candles.data.size();
        dsSlow.clear();
        dsSlow.set_window_size(slowPeriod);
        dsFast.clear();
        dsFast.set_window_size(fastPeriod);
        slowMa.resize(size);
        fastMa.resize(size);
    }

    string get_csv_header() override { return "slow,fast"; }

    string get_csv_line(int index) override {
        ostringstream oss;
        oss << slowMa[index] << "," << fastMa[index];
        return oss.str();
    }
};

#endif
