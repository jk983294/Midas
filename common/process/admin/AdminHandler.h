#ifndef MIDAS_ADMIN_HANDLER_H
#define MIDAS_ADMIN_HANDLER_H

#include <boost/exception/diagnostic_information.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <sstream>
#include <string>
#include "MidasAdminBase.h"
#include "midas/Lock.h"
#include "net/channel/Channel.h"
#include "process/admin/AdminCallback.h"
#include "process/admin/TcpAdmin.h"
#include "utils/MidasBind.h"

using namespace std;

namespace midas {

// it create a channel and waiting for incoming requests
class AdminHandler {
public:
    typedef std::map<string, AdminCallback> TCallbacks;

    size_t maxCommandLen{0};
    size_t maxDescLen{0};
    size_t maxDetailLen{0};
    LockType::mutex mtx;
    TCallbacks callbacks;
    CChannel channel;

public:
    AdminHandler() : channel("admin") {
        register_callback("net_help", MIDAS_BIND3(&AdminHandler::admin_net_help, this), "show cmd usage",
                          "net_help [command name]");
        TcpAdmin::data_callback(MIDAS_BIND2(&AdminHandler::data_notify, this));
    }

    void run() { channel.run(true); }

    void start() { channel.work(); }

    CChannel& get_channel() { return channel; }

    void register_callback(const string& cmd, const TAdminCallback& cb, CChannel& c, const string& desc,
                           const string& detail_) {
        LockType::scoped_lock guard(mtx);
        callbacks[cmd] = AdminCallback(cb, &c, desc, detail_);
        maxCommandLen = max(maxCommandLen, cmd.size());
        maxDescLen = max(maxDescLen, desc.size());
        maxDetailLen = max(maxDetailLen, detail_.size());
    }

    void register_callback(const string& cmd_, const TAdminCallback& cb, const string& desc = string(),
                           const string& detail_ = string()) {
        register_callback(cmd_, cb, channel, desc, detail_);
    }

    // request iosvc to invoke handler and return immediately
    template <typename Handler>
    void post(Handler f) {
        channel.mainStrand.post(f);
    }

    bool remove(const string& cmd) {
        LockType::scoped_lock guard(mtx);
        return callbacks.erase(cmd) != 0;
    }

    void get_help(const string& cmd, string& help) {
        LockType::scoped_lock guard(mtx);
        auto itr = callbacks.find(cmd);
        if (itr != callbacks.end()) {
            help = itr->second.detail;
        }
    }

private:
    // callback to handle coming admin request
    size_t data_notify(const ConstBuffer& msg, TcpAdmin::SharedPtr member) {
        boost::property_tree::ptree pt;
        istringstream is(string(msg.data(), msg.size()));
        boost::property_tree::read_json(is, pt);

        string command = pt.get("command", "");
        string userId = pt.get("userId", "");
        string requestId = pt.get("requestId", "");

        if (!command.empty()) {
            ostringstream os;
            boost::property_tree::ptree& v = pt.get_child("arguments");
            TAdminCallbackArgs args;
            for (auto i = v.begin(); i != v.end(); ++i) {
                args.push_back(i->second.data());
                os << " " << i->second.data();
            }

            MIDAS_LOG_INFO("receive admin command from user " << userId << " : " << command << os.str());

            {
                LockType::scoped_lock guard(mtx);
                auto itr = callbacks.find(command);
                if (itr != callbacks.end() && itr->second.channel != nullptr) {
                    itr->second.channel->mainStrand.post(boost::bind(&AdminHandler::call_cmd, this, itr->second,
                                                                     command, args, userId, requestId, member));
                    itr->second.channel->start(1);  // dispatch with at least one thread
                } else {
                    MIDAS_LOG_ERROR("invalid channel or callback for command " << command);
                    send_response("invalid command", userId, requestId, member);
                }
            }
        } else {
            send_response("invalid command", userId, requestId, member);
        }
        return msg.size();
    }

    void call_cmd(AdminCallback callback, const string& cmd, const TAdminCallbackArgs& args, const string& userId,
                  const string& requestId, TcpAdmin::SharedPtr member) {
        send_response(callback(cmd, args, userId), userId, requestId, member);
    }

    void send_response(const string& response, const string& userId, const string& requestId,
                       TcpAdmin::SharedPtr member) {
        ostringstream os;
        string res{response};
        std::replace(res.begin(), res.end(), '"', '\'');
        os << "{"
           << "\"userId\" : "
           << "\"" << userId << "\", "
           << "\"requestId\" : "
           << "\"" << requestId << "\", "
           << "\"response\" : "
           << "\"" << res << "\""
           << "}";
        channel.deliver<TcpAdmin>(os.str(), member);
    }

    string admin_net_help(const string& cmd, const TAdminCallbackArgs& args, const string& userId) {
        ostringstream os;
        os << setw(maxCommandLen + 1) << left << "command" << setw(maxDescLen + 1) << left << "description" << left
           << "help" << '\n';

        if (args.size() == 0) {
            for (auto i = callbacks.begin(); i != callbacks.end(); ++i) {
                os << setw(maxCommandLen + 1) << left << i->first << setw(maxDescLen + 1) << left
                   << i->second.description << left << i->second.detail << '\n';
            }
        } else {
            auto i = callbacks.find(cmd);
            os << setw(maxCommandLen + 1) << left << i->first << setw(maxDescLen + 1) << left << i->second.description
               << left << i->second.detail << '\n';
        }
        return os.str();
    }
};
}

#endif
