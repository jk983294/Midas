#ifndef MIDAS_STRATEGY_FACTORY_H
#define MIDAS_STRATEGY_FACTORY_H

#include <map>
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
    static std::unique_ptr<StrategyBase> create_strategy(const CtpInstrument& instrument, CandleScale scale,
                                                         StrategyType strategyType) {
        switch (strategyType) {
            case StrategyType::TMaStrategy:
                return std::make_unique<BiMaStrategy>(instrument, scale);
            case StrategyType::TBiMaStrategy:
                return std::make_unique<BiMaStrategy>(instrument, scale);
            default:
                throw "invalid strategy type.";
        }
    }

    static void set_strategy(CtpInstrument& instrument, StrategyType strategyType);

    static void set_strategy(std::map<string, std::shared_ptr<CtpInstrument>>& instruments, StrategyType strategyType);
};

#endif
