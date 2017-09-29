#include "catch.hpp"
#include "utils/log/Log.h"

using namespace midas;

TEST_CASE("log init", "[log]") { Log& log = Log::instance(); }

TEST_CASE("log msg", "[log]") {
    LogMsg m1("kun");
    REQUIRE(m1.str() == "kun");

    LogMsg m2("%s %s", "kun", "jiang");
    REQUIRE(m2.str() == "kun jiang");

    LogMsg m3;
    m3 << "hello" << 42;
    REQUIRE(m3.str() == "hello42");
}
