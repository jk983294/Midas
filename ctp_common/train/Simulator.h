#ifndef MIDAS_SIMULATOR_H
#define MIDAS_SIMULATOR_H

#include "TrainCommon.h"
#include "model/CtpData.h"
#include "model/CtpOrder.h"

class Simulator {
public:
    std::shared_ptr<CtpData> data;
    PositionManager& positionManager;
    std::vector<CtpOrder> orders;
    std::vector<std::shared_ptr<CtpInstrument>> currentRound;

public:
    Simulator(std::shared_ptr<CtpData> data_) : data(data_), positionManager(data_->positionManager) {}

    void apply(const StrategyParameter& parameter) {}

    std::shared_ptr<BacktestResult> get_performance() {
        init();
        simulate();
        std::shared_ptr<BacktestResult> result(new BacktestResult);
        result->init(positionManager);
        return result;
    }

private:
    void simulate() {
        while (true) {
            if (get_available_instrument()) {
                /**
                 * calculate market value, open new position, close old position
                 */
                positionManager.mark2market();
                std::shared_ptr<CtpInstrument> best = positionManager.new_signals(currentRound);
                if (best) {
                    positionManager.order4simulation(best);
                }
                positionManager.close_position4simulation();
                positionManager.order2position4simulation();

                advance_timestamp();
            } else {
                break;  // all instruments have finished time line advance
            }
        }
        positionManager.close_exist_position4simulation();
    }

    /**
     * find available instruments with earliest time, calculate signal
     * @return true means some instrument can simulate this time
     *         false means all instruments finish its simulation
     */
    bool get_available_instrument() {
        currentRound.clear();
        Timestamp currentEarliest;
        for (auto& item : data->instruments) {
            CtpInstrument& instrument = *item.second;
            if (instrument.strategy->is_valid_time()) {
                const midas::Timestamp& now = instrument.strategy->current_time();
                if (currentRound.empty()) {
                    currentEarliest = now;
                    currentRound.push_back(item.second);
                } else if (now < currentEarliest) {
                    currentEarliest = now;
                    currentRound.clear();
                    currentRound.push_back(item.second);
                } else {
                    currentRound.push_back(item.second);
                }
            }
        }

        if (currentRound.empty()) return false;

        for (auto& pInstrument : currentRound) {
            double marketPrice = pInstrument->strategy->current_candle().open;
            pInstrument->market_price(marketPrice);
            pInstrument->strategy->calculate_current(marketPrice);
        }
        return true;
    }

    void advance_timestamp() {
        for (auto& pInstrument : currentRound) {
            ++pInstrument->strategy->itr;
        }
    }

    void init() {
        orders.clear();
        for (auto& item : data->instruments) {
            item.second->strategy->init();
        }
        positionManager.adjust_leverage(data->instruments.size());
        positionManager.init4simulation();
    }
};

#endif
