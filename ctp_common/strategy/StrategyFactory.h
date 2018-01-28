#ifndef MIDAS_STRATEGY_FACTORY_H
#define MIDAS_STRATEGY_FACTORY_H

#include <memory>
#include "BiMaStrategy.h"
#include "StrategyBase.h"

enum StrategyType { TMaStrategy, TBiMaStrategy, TUnknown };

inline StrategyType string2strategy(const string& str) {
    if (str == "TMaStrategy")
        return StrategyType::TMaStrategy;
    else if (str == "TBiMaStrategy")
        return StrategyType::TBiMaStrategy;
    else
        return StrategyType::TUnknown;
}

class StrategyFactory {
public:
    static std::unique_ptr<StrategyBase> create_strategy(const Candles& candles_, StrategyType strategyType) {
        switch (strategyType) {
            case StrategyType::TMaStrategy:
                return std::make_unique<BiMaStrategy>(candles_);
            case StrategyType::TBiMaStrategy:
                return std::make_unique<BiMaStrategy>(candles_);
            default:
                throw "invalid strategy type.";
        }
    }

    static void set_strategy(CtpInstrument& instrument, StrategyType strategyType) {
        instrument.strategy =
            create_strategy(instrument.get_candle_reference(StrategyParameter::instance().scale), strategyType);
    }

    static void set_strategy(map<string, std::shared_ptr<CtpInstrument>>& instruments, StrategyType strategyType) {
        for (auto& item : instruments) {
            if (item.second->isMasterContract) {
                set_strategy(*(item.second), strategyType);
            }
        }
    }
};

#endif
