#ifndef MIDAS_MIDAS_ADMIN_BASE_H
#define MIDAS_MIDAS_ADMIN_BASE_H

#include <boost/function.hpp>
#include <string>
#include <vector>

using namespace std;

namespace midas {
typedef vector<string> TAdminCallbackArgs;
typedef boost::function<std::string(const std::string&, const TAdminCallbackArgs&, const std::string& userId)>
    TAdminCallback;

#define MIDAS_ADMIN_HELP_RESPONSE "default admin help response"

class AdminPortal {
public:
    virtual ~AdminPortal() {}
    virtual void register_admin(const string& cmd, const TAdminCallback& callback, const string& description = string(),
                                const string& detail = string()) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void get_help(const string& cmd, string& help) { help = MIDAS_ADMIN_HELP_RESPONSE; }
};
}

#endif  // MIDAS_MIDAS_ADMIN_BASE_H
