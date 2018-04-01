#ifndef MIDAS_STRATEGY_BASE_H
#define MIDAS_STRATEGY_BASE_H

#include <midas/Singleton.h>
#include <model/CtpDef.h>
#include "model/CandleData.h"

class CtpInstrument;  // forward declaration

struct StrategyParameter {
    CandleScale scale{CandleScale::Minute15};
    int trainStartDate{20110101};
    int trainEndDate{20141107};
    int singleInt{1};
    double singleDouble{1.0};
    double tradeTaxRate{0.003};

    static StrategyParameter& instance() { return midas::Singleton<StrategyParameter>::instance(); }
};

enum StrategyDecision { NoSignal, longSignal, shortSignal, clearLongPositionSignal, clearShortPositionSignal };
enum StrategyState { HoldMoney, HoldLongPosition, HoldShortPosition };

class StrategyBase {
public:
    int itr{0};  // candles iterator
    int coolDownCount{0};
    int singleInt{1};
    double singleDouble{1.0};
    const CtpInstrument& instrument;
    const Candles& candles;  // main candle, defined align with StrategyParameter.scale
    StrategyDecision decision{StrategyDecision::NoSignal};
    StrategyState state{StrategyState::HoldMoney};

    /**
     * value >= 100 means long signal
     * value <= -100 means short signal
     * 0 means no signal
     * (-100, 0) (0, 100) reserved
     */
    vector<int64_t> signals;

public:
    StrategyBase(const CtpInstrument& instrument_, CandleScale scale);
    virtual ~StrategyBase() = default;

    virtual void init() = 0;
    virtual void calculate(int index) = 0;

    void calculate_all() {
        init();
        for (int i = 0; i < candles.currentBinIndex; ++i) {
            calculate(i);
        }
    }

    void apply_parameter(const StrategyParameter& parameter) {
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

    bool has_signal() {
        if (itr == 0)
            return false;
        else
            return signals[itr - 1] >= signalValueBuyThreshold || signals[itr - 1] <= signalValueSellThreshold;
    }

    int64_t signal_strength() { return std::abs(signal_value()); }

    int64_t signal_value() {
        if (itr == 0)
            return 0;
        else
            return signals[itr - 1];
    }

    void set_no_signal(int i) {
        signals[i] = signalValueNo;
        decision = StrategyDecision::NoSignal;
    }

    virtual string get_csv_header() = 0;
    virtual string get_csv_line(int index) = 0;

    void csv_stream(ostream& os);
};

#endif
