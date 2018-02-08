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
    BacktestResult result;
    std::vector<std::shared_ptr<CtpInstrument>> currentRound;

public:
    Simulator(std::shared_ptr<CtpData> data_) : data(data_), positionManager(data_->positionManager) {}

    void apply(const StrategyParameter& parameter) {}

    BacktestResult get_performance() {
        init();
        simulate();
        return result;
    }

private:
    void simulate() {
        while (1) {
            if (get_available_instrument()) {
                /**
                 * calculate market value, open new position, close old position
                 */
                positionManager.mark2market(data->instruments);
                handle_existing_position();
                handle_new_position();

                advance_timestamp();
            } else {
                break;
            }
        }
    }

    void handle_existing_position() {}

    void handle_new_position() {}

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
            pInstrument->strategy->calculate_previous_candle();
        }
        return true;
    }

    void advance_timestamp() {
        for (auto& pInstrument : currentRound) {
            ++pInstrument->strategy->itr;
        }
    }

    void init() {
        memset(&result, 0, sizeof(BacktestResult));
        orders.clear();
        for (auto& item : data->instruments) {
            item.second->strategy->init();
        }
        positionManager.init4simulation();
    }
};

#endif
