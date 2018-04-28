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

    void __init();
    virtual void init() = 0;

    /**
     * calculate current candle at its beginning, so there is no low/high/close yet
     * but we do have open which passed in as marketPrice
     * @param index
     * @param marketPrice
     */
    virtual void calculate(int index, double marketPrice) = 0;

    void calculate_all();

    void apply_parameter(const StrategyParameter& parameter) {
        singleDouble = parameter.singleDouble;
        singleInt = parameter.singleInt;
    }

    bool is_valid_time() { return itr < candles.historicDataCount; }

    const CandleData& current_candle() const { return candles[itr]; }

    const CandleData& previous_candle() const { return candles[itr - 1]; }

    const CandleData& pre_pre_candle() const { return candles[itr - 2]; }

    const midas::Timestamp& current_time() const { return candles[itr].timestamp; }

    void calculate_current(double marketPrice) { calculate(itr, marketPrice); }

    bool has_signal() { return signals[itr] >= signalValueBuyThreshold || signals[itr] <= signalValueSellThreshold; }

    int64_t signal_strength() { return std::abs(signal_value()); }

    int64_t signal_value() { return signals[itr]; }

    void set_no_signal(int i) {
        signals[i] = signalValueNo;
        decision = StrategyDecision::NoSignal;
    }

    virtual string get_csv_header() = 0;
    virtual string get_csv_line(int index) = 0;

    void csv_stream(ostream& os);
};

#endif
