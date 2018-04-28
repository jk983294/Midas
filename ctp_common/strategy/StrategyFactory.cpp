#include "StrategyFactory.h"
#include "model/CtpInstrument.h"

void StrategyFactory::set_strategy(CtpInstrument& instrument, StrategyType strategyType) {
    instrument.strategy = create_strategy(instrument, StrategyParameter::instance().scale, strategyType);
}

void StrategyFactory::set_strategy(std::map<string, std::shared_ptr<CtpInstrument>>& instruments,
                                   StrategyType strategyType) {
    for (auto& item : instruments) {
        if (item.second->isMasterContract) {
            set_strategy(*(item.second), strategyType);
        }
    }
}
