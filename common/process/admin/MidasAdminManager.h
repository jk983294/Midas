#ifndef MIDAS_MIDASADMINManager_H
#define MIDAS_MIDASADMINManager_H

#include <algorithm>
#include <boost/function.hpp>
#include <string>
#include <vector>
#include "MidasAdminBase.h"
#include "midas/Lock.h"

using namespace std;

namespace midas {

class AdminCallbackManager {
public:
    typedef boost::function<void(std::ostream&, const TAdminCallbackArgs&)> AdminCallback;
    typedef vector<AdminCallback> AdminCallbackVector;
    typedef midas::LockType::mutex mutex_type;
    typedef midas::LockType::scoped_lock scoped_lock_type;

    mutable mutex_type mtx;
    AdminCallbackVector callbackVector;

public:
    AdminCallbackManager() {}
    ~AdminCallbackManager() {}

    void register_callback(const AdminCallback& callback) { callbackVector.push_back(callback); }

    void register_callback(const AdminCallbackVector& callbacks) {
        copy(callbacks.begin(), callbacks.end(), std::back_inserter(callbackVector));
    }

    string call(const string& cmd, const TAdminCallbackArgs& args) const {
        scoped_lock_type lock(mtx);

        ostringstream os;

        for (auto itr = callbackVector.begin(); itr != callbackVector.end(); ++itr) {
            size_t n = os.str().size();
            (*itr)(os, args);
            if (os.str().size() > n) os << '\n';
        }

        if (os.str().empty()) return "no data.";
        return os.str();
    }
};
}

#endif  // MIDAS_MIDASADMINBASE_H
