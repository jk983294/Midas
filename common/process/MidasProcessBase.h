#ifndef MIDAS_MIDAS_PROCESS_BASE_H
#define MIDAS_MIDAS_PROCESS_BASE_H

#include <getopt.h>
#include <syscall.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include "CommandArg.h"
#include "midas/MidasConfig.h"
#include "midas/MidasException.h"
#include "process/admin/AdminHandler.h"
#include "process/admin/MidasAdminBase.h"
#include "process/admin/MidasAdminManager.h"
#include "process/admin/TcpAdminPortal.h"
#include "utils/log/Log.h"

using namespace std;

extern char** environ;

namespace midas {

class MidasProcessBase {
public:
    sigset_t sigWaitSet;
    shared_ptr<boost::thread> signalHandler;
    string processName;
    AdminCallbackManager metersManager;
    shared_ptr<AdminPortal> portal;
    CommandArgs commandArgs;
    bool shutdown{false};
    std::mutex showdownMtx;
    std::condition_variable shutdownCondition;

public:
    void init_config(int argc, char* argv[]);
    void start();
    void run();
    void stop();
    void run_control_script();
    string admin_port() const { return Config::instance().get<string>("cmdline.admin_port"); }
    AdminHandler& admin_handler() { return dynamic_cast<TcpAdminPortal*>(portal.get())->admin_handler(); }

protected:
    MidasProcessBase(int argc, char* argv[], shared_ptr<AdminPortal> portal_ = make_shared<TcpAdminPortal>());
    MidasProcessBase() {}
    virtual ~MidasProcessBase() {}
    virtual void app_start() {}
    virtual void app_ready() {}
    virtual void app_stop() {}

    void register_command_line_args(char shortCmd_, string longCmd_, SwitchArgType switchArgType,
                                    SwitchOptionType switchOptionType, string argName_, string help_);
    void register_command_line_args(char shortCmd_, string longCmd_, bool isWithArg_, bool isOptional_, string argName_,
                                    string help_);
    void remove_command_line_arg(char shortCmd_);
    bool has_command_line_arg(char shortCmd_);

private:
    // disable copy
    MidasProcessBase(const MidasProcessBase&);
    MidasProcessBase& operator=(const MidasProcessBase&);

    // ignore SIGPIPE SIGALRM, set handler to SIGINT SIGTERM
    void setup_signal_handler();
    void on_signal();

    static string config_key_from_cmd_arg_switch(const string& longSwitch_);
    void usage(string msg = "");
    string usage_text();

    string admin_set_log_level(const string& cmd, const TAdminCallbackArgs& args);
    string admin_show_config(const string& cmd, const TAdminCallbackArgs& args);
    string admin_load_config(const string& cmd, const TAdminCallbackArgs& args);
    string admin_set_config(const string& cmd, const TAdminCallbackArgs& args);
    string admin_shutdown(const string& cmd, const TAdminCallbackArgs& args);
    string admin_get_env(const string& cmd, const TAdminCallbackArgs& args);
    void _init_admin();

    void set_log_level(int argc, char** argv) const;
};
}

#include "MidasProcessBase.inl"

#endif
