#include "catch.hpp"
#include "net/common/IpAddress.h"

using namespace midas;

TEST_CASE("ip address", "[IpAddress]") {
    string ip{"10.174.63.151"};
    {
        IpAddress address(ip);
        REQUIRE(address.ip == ip);
        IpAddress address2(address);
        REQUIRE(address.ip == address2.ip);

        IpAddress address3;
        address3 = address;
        REQUIRE(address.ip == address3.ip);
    }
}
