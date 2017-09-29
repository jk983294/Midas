#include "../helper/CtpVisualHelper.h"
#include "CtpProcess.h"
#include "net/tcp/TcpPublisherAsync.h"
#include "process/admin/MidasAdminBase.h"
#include "utils/log/Log.h"

void CtpProcess::init_admin() {
    admin_handler().register_callback("meters", boost::bind(&CtpProcess::admin_meters, this, _1, _2),
                                      "display statistical information of connections", "meters");
    admin_handler().register_callback("request", boost::bind(&CtpProcess::admin_request, this, _1, _2),
                                      "request (instrument|product|exchange|account|position)",
                                      "request data from ctp server");
    admin_handler().register_callback("query", boost::bind(&CtpProcess::admin_query, this, _1, _2),
                                      "query (instrument|product|exchange|account|position)", "query data in local");
    admin_handler().register_callback("get_async_result",
                                      boost::bind(&CtpProcess::admin_get_async_result, this, _1, _2, _3),
                                      "get_async_result", "get_async_result");
    admin_handler().register_callback("clear_async_result",
                                      boost::bind(&CtpProcess::admin_clear_async_result, this, _1, _2, _3),
                                      "clear_async_result", "clear_async_result");
    admin_handler().register_callback("buy", boost::bind(&CtpProcess::admin_buy, this, _1, _2),
                                      "buy instrument size price", "buy instrument size price");
    admin_handler().register_callback("sell", boost::bind(&CtpProcess::admin_sell, this, _1, _2),
                                      "sell instrument size price", "sell instrument size price");
    admin_handler().register_callback("close", boost::bind(&CtpProcess::admin_close, this, _1, _2),
                                      "close instrument size", "close instrument size");
}

string CtpProcess::admin_request(const string& cmd, const TAdminCallbackArgs& args) {
    string param1, param2;
    if (args.size() > 0) param1 = args[0];
    if (args.size() > 1) param2 = args[1];
    ostringstream oss;
    if (param1 == "instrument") {
        manager->query_instrument(param2);
    } else if (param1 == "product") {
        manager->query_product(param2);
    } else if (param1 == "") {
        oss << "unknown parameter";
    }
    return oss.str();
}

string CtpProcess::admin_query(const string& cmd, const TAdminCallbackArgs& args) {
    string param1, param2;
    if (args.size() > 0) param1 = args[0];
    if (args.size() > 1) param2 = args[1];
    ostringstream oss;
    if (param1 == "instrument") {
        if (param2 == "" || param2 == "all") {
            for (auto it = data.instruments.begin(); it != data.instruments.end(); ++it) oss << it->second << endl;
        } else if (data.instruments.find(param2) != data.instruments.end()) {
            oss << data.instruments[param2];
        } else {
            oss << "can not find instrument for " << param2;
        }
    } else if (param1 == "product") {
        if (param2 == "" || param2 == "all") {
            for (auto it = data.products.begin(); it != data.products.end(); ++it) oss << it->second << endl;
        } else if (data.products.find(param2) != data.products.end()) {
            oss << data.products[param2];
        } else {
            oss << "can not find instrument for " << param2;
        }
    } else if (param1 == "") {
        oss << "unknown parameter";
    }
    return oss.str();
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
    ChMapSS::accessor a;
    if (data.user2asyncData.find(a, userId)) {
        oss << a->second << endl;
        data.user2asyncData.erase(a);
    } else {
        oss << "no async result for user " << userId;
    }
    return oss.str();
}

string CtpProcess::admin_clear_async_result(const string& cmd, const TAdminCallbackArgs& args,
                                            const std::string& userId) {
    ostringstream oss;
    ChMapSS::accessor a;
    if (data.user2asyncData.find(a, userId)) {
        data.user2asyncData.erase(a);
        oss << "clear async result for user " << userId;
    } else {
        oss << "no async result for user " << userId;
    }
    return oss.str();
}

string CtpProcess::admin_buy(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    if (args.size() == 0)
        oss << "missing parameter\n";
    else {
        string content = args[0];
        oss << "parameter: " << content << endl;
    }
    return oss.str();
}

string CtpProcess::admin_sell(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    if (args.size() == 0)
        oss << "missing parameter\n";
    else {
        string content = args[0];
        oss << "parameter: " << content << endl;
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
    oss << "clients:\n";
    {
        string sep, tmp = oss.str(), delimiter{"\n"};
        string::size_type pos = 0;
        delimiter.append(96, '_');
        auto beg = tmp.rfind(delimiter);
        if (string::npos != beg) {
            pos = beg + 1;
            auto lf = tmp.find('\n', pos);
            sep = tmp.substr(pos, string::npos == lf ? lf : lf - beg);
            if (lf + 1 >= tmp.length()) {
                oss << "no connections\n";
            }
        }
        oss << sep;
    }
    return oss.str();
}