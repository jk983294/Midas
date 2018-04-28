#include "StrategyBase.h"
#include "model/CtpInstrument.h"

StrategyBase::StrategyBase(const CtpInstrument& instrument_, CandleScale scale)
    : instrument(instrument_), candles(instrument.get_candle_reference(scale)) {}

void StrategyBase::__init() {
    itr = 0;
    coolDownCount = 0;
    size_t size = candles.data.size();
    signals.resize(size);
    fill(signals.begin(), signals.end(), 0);
}

void StrategyBase::calculate_all() {
    init();
    for (int i = 0; i < candles.currentBinIndex; ++i) {
        calculate(i, candles[i].open);
    }
}

void StrategyBase::csv_stream(ostream& os) {
    os << "date,time,open,high,low,close,volume,signals," << get_csv_header() << '\n';
    int totalAvailableCount = candles.total_available_count();
    for (int i = 0; i < totalAvailableCount; ++i) {
        os << candles.data[i] << ',' << signals[i] << ',' << get_csv_line(i) << '\n';
    }
}
