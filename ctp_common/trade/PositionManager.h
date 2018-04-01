#ifndef MIDAS_POSITION_MANAGER_H
#define MIDAS_POSITION_MANAGER_H

#include <model/CtpInstrument.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "model/CtpOrder.h"

class PositionManager {
public:
    // TODO consider unfilled order take margin in the future
    std::unordered_map<int, std::shared_ptr<CtpOrder>> signalledOrders;
    std::vector<std::shared_ptr<CtpPosition>> portfolio;
    std::vector<std::shared_ptr<CtpPosition>> finished;
    int orderId;
    int maxInstrument{splitFactor};
    double totalInstrumentValue{0};
    double totalAsset{0};  //  = cash + totalMargin
    double cash{0};        // = sum(position.deltaProfit) - sum(position.deltaMargin)
    double totalProfit{0};
    double totalMargin{0};
    std::mutex mtx;

public:
    PositionManager() = default;

    void mark2market();

    std::shared_ptr<CtpInstrument> new_signals(const std::vector<std::shared_ptr<CtpInstrument>>& currentRound);

    void adjust_leverage(int maxInstrumentCount) { maxInstrument = std::min(maxInstrumentCount, splitFactor); }

    void add_position(std::shared_ptr<CtpPosition> position);
    void close_position(std::shared_ptr<CtpPosition> position);

public:
    void init4simulation();
    void order4simulation(const std::shared_ptr<CtpInstrument>& pInstrument);
    void order2position4simulation();
    void close_position4simulation();
    void close_exist_position4simulation();

private:
    bool has_exist_position(const std::shared_ptr<CtpInstrument>& pInstrument);

    bool better_position_candidate(const std::shared_ptr<CtpInstrument>& best,
                                   const std::shared_ptr<CtpInstrument>& candidate);

    int current_position_count();

    void remove_closed_position();
};

#endif
