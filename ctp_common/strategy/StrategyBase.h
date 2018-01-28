#ifndef MIDAS_STRATEGY_BASE_H
#define MIDAS_STRATEGY_BASE_H

#include "model/CandleData.h"

struct StrategyParameter {
    CandleScale scale{CandleScale::Minute15};
    int trainStartDate{20110101};
    int trainEndDate{20141107};
    int singleInt{1};
    double singleDouble{1.0};
    double tradeTaxRate{0.003};

    static StrategyParameter& instance() { return midas::Singleton<StrategyParameter>::instance(); }
};

class StrategyBase {
public:
    int singleInt{1};
    double singleDouble{1.0};
    const Candles& candles;
    vector<double> signals;

public:
    StrategyBase(const Candles& candles_) : candles(candles_) {}
    virtual ~StrategyBase() = default;

    virtual void init() = 0;
    virtual void calculate(size_t index) = 0;

    void calculate_all() {
        init();
        for (size_t i = 0; i < candles.currentBinIndex; ++i) {
            calculate(i);
        }
    }

    void apply_parameter(StrategyParameter parameter) {
        singleDouble = parameter.singleDouble;
        singleInt = parameter.singleInt;
    }

    virtual string get_csv_header() = 0;
    virtual string get_csv_line(size_t index) = 0;

    void csv_stream(ostream& os) {
        os << "date,time,open,high,low,close,volume," << get_csv_header() << '\n';
        size_t endPos = candles.currentBinIndex;
        if (candles.data[candles.currentBinIndex].date != 0) ++endPos;

        for (size_t i = 0; i < endPos; ++i) {
            os << candles.data[i] << "," << get_csv_line(i) << '\n';
        }
    }
};

#endif
