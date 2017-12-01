#include "catch.hpp"
#include "midas/MidasConfig.h"

using namespace midas;

TEST_CASE("get set", "[config]") {
    const string port = Config::instance().get<string>("cmd.server_port", "8023");
    REQUIRE(port == "8023");

    bool data = Config::instance().get("test", true);
    REQUIRE(data == true);

    Config::instance().put("test", false);
    REQUIRE(Config::instance().get<bool>("test") == false);
    REQUIRE(Config::instance().get<string>("test") == "false");

    Config::instance().put("double", 1.2);
    REQUIRE(Config::instance().get<double>("double") == 1.2);

    {
        REQUIRE(setenv("env_data", "42", 1) == 0);
        REQUIRE(Config::instance().get("env_data", "non42") == "42");
    }

    Config::instance().put("a.1", "a");
    Config::instance().put("b.1", 42);
    Config::instance().put("c.1", "contains substitution $(a.1) and $(b.1)");
    REQUIRE(Config::instance().get<string>("c.1") == "contains substitution a and 42");
}

TEST_CASE("get set in file", "[config]") {
    string file("/tmp/config_test.info");
    ofstream ofs(file);
    ofs << "; A comment\n"
           "key1 value1   ; Another comment\n"
           "key2 \"value with special characters in it {};#\\n\\t\\\"\\0\"\n"
           "{\n"
           "   subkey \"value split \"\\\n"
           "          \"over three\"\\\n"
           "          \"lines\"\n"
           "   {\n"
           "      a_key_without_value \"\"\n"
           "      \"a key with special characters in it {};#\\n\\t\\\"\\0\" \"\"\n"
           "   }\n"
           "}"
        << '\n';
    ofs.close();

    Config::instance().read_config(file);
    REQUIRE(Config::instance().get("key1", "value2") == "value1");
    REQUIRE(Config::instance().get("key2.subkey.a_key_without_value", "value2") == "");
}
