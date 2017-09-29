#include "catch.hpp"
#include "net/common/NetworkHelper.h"

using namespace midas;

TEST_CASE("NetworkHelper", "[NetworkHelper]") {
    NetworkHelper helper;
    IpAddress address;
    string interface{"lo"};
    helper.lookup_interface(interface, address);
    REQUIRE(address.ip == "127.0.0.1");

    string subnetAndBits = "238.255.32.128/25";
    string subnet;
    int bits;
    NetworkHelper::parse_subnet_and_bits(subnetAndBits, subnet, bits);
    REQUIRE(subnet == "238.255.32.128");
    REQUIRE(bits == 25);

    subnetAndBits = "127.0.0.1";
    NetworkHelper::parse_subnet_and_bits(subnetAndBits, subnet, bits);
    REQUIRE(subnet == "127.0.0.1");
    REQUIRE(bits == 32);

    string interfaceFound;
    helper.lookup_interface_from_subnet("127.0.0.0", 24, interfaceFound);
    REQUIRE(interfaceFound == interface);

    {
        string interfaceIp;
        string interfaceConfig{"127.0.0.1"};
        helper.lookup_interface(interfaceConfig, interfaceIp);
        REQUIRE(interfaceIp == "127.0.0.1");
    }
    {
        string interfaceIp;
        string interfaceConfig{"127.0.0.0/24"};
        helper.lookup_interface(interfaceConfig, interfaceIp);
        REQUIRE(interfaceIp == "127.0.0.1");
    }
    {
        string interfaceIp;
        string interfaceConfig{interface};
        helper.lookup_interface(interfaceConfig, interfaceIp);
        REQUIRE(interfaceIp == "127.0.0.1");
    }
    {
        string interfaceIp;
        string interfaceConfig{"128.0.0.1"};

        try {
            helper.lookup_interface(interfaceConfig, interfaceIp);
            REQUIRE(false);  // won't get here
        } catch (std::exception& e) {
            REQUIRE(true);
        }
    }

    {
        string interfaceIp;
        string interfaceConfig{"lo10"};
        try {
            helper.lookup_interface(interfaceConfig, interfaceIp);
            REQUIRE(false);  // won't get here
        } catch (std::exception& e) {
            REQUIRE(true);
        }
    }
}

TEST_CASE("split_host_port_interface", "[NetworkHelper]") {
    string addr{":8023"};
    string host, port, itf;
    split_host_port_interface(addr, host, port, itf);
    REQUIRE(host == "localhost");
    REQUIRE(port == "8023");
    REQUIRE(itf == "");

    addr = "a:8023:interface1";
    host = port = itf = "";
    split_host_port_interface(addr, host, port, itf);
    REQUIRE(host == "a");
    REQUIRE(port == "8023");
    REQUIRE(itf == "interface1");

    addr = "a:8023";
    host = port = itf = "";
    split_host_port_interface(addr, host, port, itf);
    REQUIRE(host == "a");
    REQUIRE(port == "8023");
    REQUIRE(itf == "");

    uint16_t portNum;
    addr = "a:8023:interface1";
    host = port = itf = "";
    split_host_port_interface(addr, host, portNum, itf);
    REQUIRE(host == "a");
    REQUIRE(portNum == 8023);
    REQUIRE(itf == "interface1");
}
