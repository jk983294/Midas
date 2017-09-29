#ifndef MIDAS_ADMIN_CALLBACK_H
#define MIDAS_ADMIN_CALLBACK_H

#include "net/channel/Channel.h"
#include "process/admin/MidasAdminBase.h"

using namespace std;

namespace midas {

struct AdminCallback {
    TAdminCallback callback;
    CChannel* channel;
    string description;
    string detail;

    AdminCallback(const TAdminCallback& cb = TAdminCallback(), CChannel* c = nullptr, const string& desc = string(),
                  const string& detail_ = string())
        : callback(cb), channel(c), description(desc), detail(detail_) {}

    string operator()(const string& cmd, const TAdminCallbackArgs& args, const string& userId) {
        string ret;
        try {
            if (callback) {
                ret = callback(cmd, args, userId);
                if (ret == MIDAS_ADMIN_HELP_RESPONSE) {
                    ret = detail;
                } else if (ret.empty()) {
                    ret = string{"warning: admin get no response"};
                }
            }
        } catch (const std::exception& e) {
            MIDAS_LOG_ERROR("exception caught: " << e.what());
            ret = e.what();
        } catch (...) {
            ostringstream os;
            os << "error: unhandled exception caught: " << boost::current_exception_diagnostic_information();
            MIDAS_LOG_ERROR(os.str());
            ret = os.str();
        }
        return ret;
    }
};
}

#endif  // MIDAS_ADMIN_CALLBACK_H
