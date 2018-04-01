#include <strategy/StrategyFactory.h>
#include <trade/PositionManager.h>
#include <catch.hpp>
#include "utils/math/MathHelper.h"

using namespace midas;

TEST_CASE("position", "[CtpOrder]") {
    TradeSessions dummy;
    std::shared_ptr<CThostFtdcInstrumentField> field(new CThostFtdcInstrumentField);
    field->VolumeMultiple = 5;
    field->PriceTick = 10;
    field->ShortMarginRatio = 0.07;
    field->LongMarginRatio = 0.07;
    std::shared_ptr<CtpInstrument> instrument = make_shared<CtpInstrument>("cu1803", dummy);
    instrument->info = field;
    instrument->market_price(52700);
    StrategyFactory::set_strategy(*instrument, StrategyType::TBiMaStrategy);

    std::shared_ptr<CtpPosition> position(new CtpPosition(instrument, CtpDirection::Long, 20, 52700));

    PositionManager manager;
    manager.init4simulation();
    manager.add_position(position);

    double commissionFee1 = position->get_commission();

    REQUIRE(is_equal(manager.cash, 631100 - commissionFee1));
    REQUIRE(is_equal(manager.totalAsset, InitAsset - commissionFee1));
    REQUIRE(is_equal(manager.totalProfit, -commissionFee1));
    REQUIRE(is_equal(manager.totalMargin, 368900));

    REQUIRE(is_equal(position->profit, -commissionFee1));
    REQUIRE(is_equal(position->deltaProfit, 0));
    REQUIRE(is_equal(position->totalInstrumentValue, 5270000));
    REQUIRE(is_equal(position->margin, 368900));
    REQUIRE(is_equal(position->deltaMargin, 0));

    instrument->market_price(52950);  // change market price
    manager.mark2market();

    REQUIRE(is_equal(manager.cash, 654350 - commissionFee1));
    REQUIRE(is_equal(manager.totalAsset, InitAsset + 25000 - commissionFee1));
    REQUIRE(is_equal(manager.totalProfit, 25000 - commissionFee1));
    REQUIRE(is_equal(manager.totalMargin, 370650));

    //    REQUIRE(position->deltaProfit == 25000);
    REQUIRE(is_equal(position->profit, 25000 - commissionFee1));
    REQUIRE(is_equal(position->deltaProfit, 25000));
    REQUIRE(is_equal(position->totalInstrumentValue, 5295000));
    REQUIRE(is_equal(position->margin, 370650));
    REQUIRE(is_equal(position->deltaMargin, 1750));

    double commissionFee2 = position->get_commission();
    manager.close_position(position);

    REQUIRE(is_equal(manager.cash, 654350 + 370650 - commissionFee1 - commissionFee2));
    REQUIRE(is_equal(manager.totalAsset, InitAsset + 25000 - commissionFee1 - commissionFee2));
    REQUIRE(is_equal(manager.totalProfit, 0));
    REQUIRE(is_equal(manager.totalMargin, 0));

    REQUIRE(is_equal(position->profit, 25000 - commissionFee1 - commissionFee2));
    REQUIRE(is_equal(position->deltaProfit, 0));
    REQUIRE(is_equal(position->totalInstrumentValue, 5295000));
    REQUIRE(is_equal(position->margin, 370650));
    REQUIRE(is_equal(position->deltaMargin, 0));
}
