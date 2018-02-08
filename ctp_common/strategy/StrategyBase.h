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

enum StrategyDecision { longSignal, shortSignal, clearPositionSignal, holdPosition, holdEmpty };

class StrategyBase {
public:
    int itr{0};
    int singleInt{1};
    double singleDouble{1.0};
    const Candles& candles;
    StrategyDecision decision{StrategyDecision::holdEmpty};

    /**
     * value > 0.5 means long signal
     * value < -0.5 means short signal
     * other value means no signal
     */
    vector<double> signals;

public:
    StrategyBase(const Candles& candles_) : candles(candles_) {}
    virtual ~StrategyBase() = default;

    virtual void init() = 0;
    virtual void calculate(size_t index) = 0;

    void calculate_all() {
        init();
        for (int i = 0; i < candles.currentBinIndex; ++i) {
            calculate(i);
        }
    }

    void apply_parameter(StrategyParameter parameter) {
        singleDouble = parameter.singleDouble;
        singleInt = parameter.singleInt;
    }

    bool is_valid_time() { return itr < candles.historicDataCount; }

    const CandleData& current_candle() const { return candles.get(itr); }

    const CandleData& previous_candle() const { return candles.get(itr - 1); }

    const midas::Timestamp& current_time() const { return candles.get(itr).timestamp; }

    void calculate_previous_candle() {
        if (itr == 0) return;
        calculate(itr - 1);
    }

    virtual string get_csv_header() = 0;
    virtual string get_csv_line(size_t index) = 0;

    void csv_stream(ostream& os) {
        os << "date,time,open,high,low,close,volume,signals," << get_csv_header() << '\n';
        int totalAvailableCount = candles.total_available_count();
        for (int i = 0; i < totalAvailableCount; ++i) {
            os << candles.data[i] << ',' << signals[i] << ',' << get_csv_line(i) << '\n';
        }
    }
};

#endif
