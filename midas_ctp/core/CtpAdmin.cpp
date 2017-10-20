#include <utils/FileUtils.h>
#include "../helper/CtpVisualHelper.h"
#include "CtpProcess.h"

void CtpProcess::init_admin() {
    admin_handler().register_callback("meters", boost::bind(&CtpProcess::admin_meters, this, _1, _2),
                                      "display statistical information of connections", "meters");
    admin_handler().register_callback("request", boost::bind(&CtpProcess::admin_request, this, _1, _2),
                                      "request (instrument|product|exchange|account|position)",
                                      "request data from ctp server");
    admin_handler().register_callback("query", boost::bind(&CtpProcess::admin_query, this, _1, _2),
                                      "query (book|image|instrument|product|exchange|account|position)",
                                      "query data in local memory");
    admin_handler().register_callback("dump", boost::bind(&CtpProcess::admin_dump, this, _1, _2),
                                      "dump (instrument|product|exchange|account|position)", "dump data");
    admin_handler().register_callback("get_async_result",
                                      boost::bind(&CtpProcess::admin_get_async_result, this, _1, _2, _3),
                                      "get_async_result", "get_async_result");
    admin_handler().register_callback("clear_async_result",
                                      boost::bind(&CtpProcess::admin_clear_async_result, this, _1, _2, _3),
                                      "clear_async_result", "clear_async_result");
    admin_handler().register_callback("buy", boost::bind(&CtpProcess::admin_buy, this, _1, _2),
                                      "buy instrument price size", "buy in limit order manner");
    admin_handler().register_callback("sell", boost::bind(&CtpProcess::admin_sell, this, _1, _2),
                                      "sell instrument price size", "sell in limit order manner");
    admin_handler().register_callback("close", boost::bind(&CtpProcess::admin_close, this, _1, _2),
                                      "close instrument size", "close instrument size");
}

string CtpProcess::admin_request(const string& cmd, const TAdminCallbackArgs& args) {
    string param1, param2;
    if (args.size() > 0) param1 = args[0];
    if (args.size() > 1) param2 = args[1];

    if (param1 == "instrument") {
        manager->query_instrument(param2);
    } else if (param1 == "product") {
        manager->query_product(param2);
    } else if (param1 == "exchange") {
        manager->query_exchange(param2);
    } else if (param1 == "account") {
        manager->query_trading_account();
    } else if (param1 == "position") {
        data->positions.clear();
        manager->query_position(param2);
    } else if (param1 == "") {
        return "unknown parameter";
    }
    return "request sent";
}

string CtpProcess::admin_query(const string& cmd, const TAdminCallbackArgs& args) {
    string param1, param2;
    if (args.size() > 0) param1 = args[0];
    if (args.size() > 1) param2 = args[1];
    ostringstream oss;
    if (param1 == "book") {
        data->books.stream(oss, param2, false);
    } else if (param1 == "image") {
        data->books.stream(oss, param2, true);
    } else if (param1 == "instrument") {
        dump2stream(oss, data->instruments, param2);
    } else if (param1 == "product") {
        dump2stream(oss, data->products, param2);
    } else if (param1 == "exchange") {
        dump2stream(oss, data->exchanges, param2);
    } else if (param1 == "account") {
        dump2stream(oss, data->accounts, param2);
    } else if (param1 == "position") {
        oss << data->positions << endl;
    } else if (param1 == "") {
        oss << "unknown parameter";
    }
    return oss.str();
}

string CtpProcess::admin_dump(const string& cmd, const TAdminCallbackArgs& args) {
    string param1, param2;
    if (args.size() > 0) param1 = args[0];
    if (args.size() > 1) param2 = args[1];

    if (param1 == "instrument" || param1 == "" || param1 == "all")
        dump2file(data->instruments, data->dataDirectory + "/instrument.dump");
    if (param1 == "product" || param1 == "" || param1 == "all")
        dump2file(data->products, data->dataDirectory + "/product.dump");
    if (param1 == "exchange" || param1 == "" || param1 == "all")
        dump2file(data->exchanges, data->dataDirectory + "/exchange.dump");
    if (param1 == "account" || param1 == "" || param1 == "all")
        dump2file(data->accounts, data->dataDirectory + "/account.dump");
    if (param1 == "position" || param1 == "" || param1 == "all")
        dump2file(data->positions, data->dataDirectory + "/position.dump");
    return "dump finished";
}

string CtpProcess::admin_get_async_result(const string& cmd, const TAdminCallbackArgs& args,
                                          const std::string& userId) {
    ostringstream oss;
    if (args.size() == 0)
        oss << "missing parameter\n";
    else {
        string content = args[0];
        oss << "user: " << userId << "parameter: " << content;
    }
    CtpData::TMapSS::accessor a;
    if (data->user2asyncData.find(a, userId)) {
        oss << a->second << endl;
        data->user2asyncData.erase(a);
    } else {
        oss << "no async result for user " << userId;
    }
    return oss.str();
}

string CtpProcess::admin_clear_async_result(const string& cmd, const TAdminCallbackArgs& args,
                                            const std::string& userId) {
    ostringstream oss;
    CtpData::TMapSS::accessor a;
    if (data->user2asyncData.find(a, userId)) {
        data->user2asyncData.erase(a);
        oss << "clear async result for user " << userId;
    } else {
        oss << "no async result for user " << userId;
    }
    return oss.str();
}

string CtpProcess::admin_buy(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    if (args.size() < 3)
        oss << "missing parameter: buy instrument price size";
    else {
        string instrument = args[0];
        double price = boost::lexical_cast<double>(args[1]);
        int size = boost::lexical_cast<int>(args[2]);
        oss << "parameter: " << instrument << " " << price << " " << size << endl;
        int ret = manager->request_buy(instrument, price, size);
        if (ret == 0) {
            oss << "buy order sent success for " << instrument << " " << price << " " << size << endl;
        } else {
            oss << "buy order sent failed with error code " << ret << endl;
        }
    }
    return oss.str();
}

string CtpProcess::admin_sell(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    if (args.size() < 3)
        oss << "missing parameter: sell instrument price size";
    else {
        string instrument = args[0];
        double price = boost::lexical_cast<double>(args[1]);
        int size = boost::lexical_cast<int>(args[2]);
        oss << "parameter: " << instrument << " " << price << " " << size << endl;
        int ret = manager->request_sell(instrument, price, size);
        if (ret == 0) {
            oss << "sell order sent success for " << instrument << " " << price << " " << size << endl;
        } else {
            oss << "sell order sent failed with error code " << ret << endl;
        }
    }
    return oss.str();
}

string CtpProcess::admin_close(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    if (args.size() == 0)
        oss << "missing parameter\n";
    else {
        string content = args[0];
        oss << "parameter: " << content << endl;
    }
    return oss.str();
}

string CtpProcess::admin_meters(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    if (disruptorPtr) disruptorPtr->stats(oss);
    if (consumerPtr) consumerPtr->stats(oss);
    return oss.str();
}
