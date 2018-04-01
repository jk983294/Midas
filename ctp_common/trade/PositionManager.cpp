#include "PositionManager.h"

using namespace std;

void PositionManager::mark2market() {
    totalInstrumentValue = 0;
    for (auto& position : portfolio) {
        position->mark2market();
        cash += (position->deltaProfit - position->deltaMargin);
        totalMargin += position->deltaMargin;
        totalProfit += position->deltaProfit;
        totalAsset += position->deltaProfit;
        totalInstrumentValue += position->totalInstrumentValue;
    }
}

std::shared_ptr<CtpInstrument> PositionManager::new_signals(
    const std::vector<std::shared_ptr<CtpInstrument>>& currentRound) {
    std::shared_ptr<CtpInstrument> best;
    for (auto& pInstrument : currentRound) {
        if (pInstrument->isMasterContract && pInstrument->strategy->has_signal() && !has_exist_position(pInstrument)) {
            if (!best)
                best = pInstrument;
            else if (better_position_candidate(best, pInstrument)) {
                best = pInstrument;
            }
        }
    }
    return best;
}

void PositionManager::init4simulation() {
    totalAsset = cash = InitAsset;
    totalProfit = totalMargin = 0;
    portfolio.clear();
    signalledOrders.clear();
    finished.clear();
}

void PositionManager::order4simulation(const std::shared_ptr<CtpInstrument>& pInstrument) {
    double price = pInstrument->market_price();
    CtpDirection dir{CtpDirection::Long};
    if (pInstrument->strategy->signal_value() < 0) {
        dir = CtpDirection::Short;
    }
    double instrumentValueLeft = maxLeverage * (cash + totalMargin) - totalInstrumentValue;
    if (instrumentValueLeft > 0) {
        int positionCountLeft = maxInstrument - current_position_count();
        if (positionCountLeft > 0) {
            int ts = static_cast<int>((instrumentValueLeft / positionCountLeft) /
                                      (price * pInstrument->info->VolumeMultiple));
            if (ts > 0) {
                std::shared_ptr<CtpOrder> order(new CtpOrder(pInstrument, dir, ts, price));
                order->id = orderId++;
                signalledOrders.insert({order->id, order});
            }
        }
    }
}

void PositionManager::order2position4simulation() {
    for (auto& item : signalledOrders) {
        auto& pOrder = item.second;
        std::shared_ptr<CtpPosition> position = std::make_shared<CtpPosition>(
            pOrder->instrument, pOrder->direction, pOrder->requestSize, pOrder->requestPrice);
        add_position(position);
    }
    signalledOrders.clear();
}

void PositionManager::close_position4simulation() {
    for (auto& position : portfolio) {
        CtpInstrument& instrument = *position->instrument;
        if (position->direction == CtpDirection::Long) {
            if (instrument.strategy->decision == StrategyDecision::shortSignal) {
                close_position(position);
            }
        } else if (position->direction == CtpDirection::Short) {
            if (instrument.strategy->decision == StrategyDecision::longSignal) {
                close_position(position);
            }
        }
    }

    remove_closed_position();
}

void PositionManager::add_position(std::shared_ptr<CtpPosition> position) {
    portfolio.push_back(position);
    totalMargin += position->margin;
    cash -= position->margin;
    totalInstrumentValue += position->totalInstrumentValue;

    // reduce commission when add into portfolio
    cash += position->profit;
    totalAsset += position->profit;
    totalProfit += position->profit;
}

void PositionManager::close_position(std::shared_ptr<CtpPosition> position) {
    double commissionFee = position->get_commission();
    cash += position->margin;
    cash -= commissionFee;
    totalMargin -= position->margin;
    totalAsset -= commissionFee;
    totalProfit -= position->profit;

    position->set_close();
    finished.push_back(position);
}

bool PositionManager::has_exist_position(const std::shared_ptr<CtpInstrument>& pInstrument) {
    for (auto& pPosition : portfolio) {
        if (pInstrument->productName == pPosition->instrument->productName) {
            MIDAS_LOG_DEBUG("has position already " << pPosition->instrument->instrumentName);
            return true;
        }
    }
    for (auto& order : signalledOrders) {
        if (pInstrument->productName == order.second->instrument->productName) {
            MIDAS_LOG_DEBUG("has position order already " << order.second->instrument->instrumentName);
            return true;
        }
    }
    return false;
}

bool PositionManager::better_position_candidate(const std::shared_ptr<CtpInstrument>& best,
                                                const std::shared_ptr<CtpInstrument>& candidate) {
    return candidate->strategy->signal_strength() > best->strategy->signal_strength();
}

int PositionManager::current_position_count() {
    int cnt = 0;
    for (auto& pPosition : portfolio) {
        if (!pPosition->closed) ++cnt;
    }
    cnt += signalledOrders.size();
    return cnt;
}

void PositionManager::remove_closed_position() {
    auto newEnd = std::remove_if(portfolio.begin(), portfolio.end(),
                                 [](const std::shared_ptr<CtpPosition>& p) { return p->closed; });
    portfolio.erase(newEnd, portfolio.end());
}

void PositionManager::close_exist_position4simulation() {
    for (auto& pPosition : portfolio) {
        pPosition->instrument->market_price(pPosition->instrument->strategy->previous_candle().close);
        close_position(pPosition);
    }
    remove_closed_position();
}
