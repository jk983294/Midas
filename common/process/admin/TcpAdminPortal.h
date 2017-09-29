#ifndef MIDAS_MIDAS_TCP_ADMIN_H
#define MIDAS_MIDAS_TCP_ADMIN_H

#include <sstream>
#include "MidasAdminBase.h"
#include "boost/make_shared.hpp"
#include "midas/MidasConfig.h"
#include "net/tcp/TcpAcceptor.h"
#include "process/admin/AdminHandler.h"
#include "process/admin/TcpAdmin.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {

class TcpAdminPortal : public AdminPortal {
public:
    typedef TcpAcceptor<TcpAdmin> AdminAcceptor;
    typedef AdminAcceptor::SharedPtr AdminAcceptorPtr;

    AdminHandler adminHandler;
    AdminAcceptorPtr adminAcceptorPtr;

public:
    TcpAdminPortal() {}
    ~TcpAdminPortal() {}

    void register_admin(const string& cmd, const TAdminCallback& callback, const string& description = string(),
                        const string& detail = string()) {
        adminHandler.register_callback(cmd, callback, description, detail);
    }

    bool start() {
        string port = admin_port();
        if (!port.empty()) {
            try {
                static size_t idCount = 0;
                ostringstream oss;
                oss << "admin" << idCount++;
                adminAcceptorPtr = AdminAcceptor::new_instance(port, adminHandler.channel, true, string(), oss.str());
                if (!adminAcceptorPtr) {
                    MIDAS_LOG_ERROR("failed to start tcp admin on port " << port);
                    return false;
                }
                adminHandler.start();  // non blocking, run admin in its own thread
            } catch (const std::exception& e) {
                MIDAS_LOG_ERROR("failed to start tcp admin on port " << port << " reason: " << e.what());
                return false;
            }
        } else {
            MIDAS_LOG_ERROR("failed to start tcp admin on port " << port << " reason: no port specified");
            return false;
        }
        return true;
    }
    void stop() { adminHandler.get_channel().stop(); }
    void get_help(const string& cmd, string& help) { adminHandler.get_help(cmd, help); }

    AdminHandler& admin_handler() { return adminHandler; }
    string admin_port() const { return Config::instance().get<string>("cmdline.admin_port"); }
};
}

#endif  // MIDAS_MIDASADMINBASE_H
