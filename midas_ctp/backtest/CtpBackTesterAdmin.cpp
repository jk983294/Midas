#include "CtpBackTester.h"

void CtpBackTester::init_admin() {
    admin_handler().register_callback("meters", boost::bind(&CtpBackTester::admin_meters, this, _1, _2),
                                      "display statistical information", "meters");
}

string CtpBackTester::admin_meters(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    return oss.str();
}
