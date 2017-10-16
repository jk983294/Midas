#ifndef MIDAS_MIDAS_PROCESS_BASE_INL
#define MIDAS_MIDAS_PROCESS_BASE_INL

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <cstring>
#include "midas/MidasConstants.h"
#include "utils/ConvertHelper.h"
#include "utils/MidasUtils.h"

using namespace std;

namespace midas {

inline MidasProcessBase::MidasProcessBase(int argc, char* argv[], shared_ptr<AdminPortal> portal_)
    : processName(argc ? basename(argv[0]) : ""), portal(portal_) {
    const string& affinity = Config::instance().getenv<std::string>("MIDAS_CPU_AFFINITY", "");
    if (!affinity.empty()) {
        set_cpu_affinity(getpid(), affinity);
    }

    setup_signal_handler();  // setup before thread run

    {
        LogPriority priority =
            from_string_log_priority(Config::instance().getenv<string>("MIDAS_LOG_LEVEL", "INFO").c_str());
        StdProcessLogWriter* writer = new StdProcessLogWriter;
        writer->init(processName);  // call StdProcessFormat.init
        MIDAS_LOG_INIT(priority, writer);
    }

    register_command_line_args('C', "config", SwitchWithArg, SwitchOptional, "config path", "load config file");
    register_command_line_args('c', "control_script", SwitchWithArg, SwitchOptional, "control script",
                               "run control script on the fly");
    register_command_line_args('a', "admin_port", SwitchWithArg, SwitchOptional, "admin port",
                               "change listening admin port");
    register_command_line_args('L', "loglevel", SwitchWithArg, SwitchOptional, "set log level", "set log level");

    _init_admin();
}

inline void MidasProcessBase::register_command_line_args(char shortCmd_, string longCmd_, SwitchArgType switchArgType,
                                                         SwitchOptionType switchOptionType, string argName_,
                                                         string help_) {
    register_command_line_args(shortCmd_, longCmd_, switchArgType == SwitchWithArg, switchOptionType == SwitchOptional,
                               argName_, help_);
}
inline void MidasProcessBase::register_command_line_args(char shortCmd_, string longCmd_, bool isWithArg_,
                                                         bool isOptional_, string argName_, string help_) {
    std::pair<CommandArgs::iterator, bool> p = commandArgs.insert(std::make_pair(
        shortCmd_,
        std::shared_ptr<CommandArg>(new CommandArg(shortCmd_, longCmd_, isWithArg_, isOptional_, argName_, help_))));

    if (!p.second) {
        THROW_MIDAS_EXCEPTION("unable to register command with short switch already used. (" << shortCmd_ << ")");
    }

    if (!isWithArg_ && !Config::instance().set<bool>(config_key_from_cmd_arg_switch(longCmd_), false)) {
        commandArgs.erase(p.first);
        THROW_MIDAS_EXCEPTION("unable to register command with long switch already used. (" << longCmd_ << ")");
    }
}

inline void MidasProcessBase::remove_command_line_arg(char shortCmd_) {
    auto itr = commandArgs.find(shortCmd_);
    if ((*itr).second->isOptional) {
        commandArgs.erase(itr);
    }
}
inline bool MidasProcessBase::has_command_line_arg(char shortCmd_) {
    return commandArgs.find(shortCmd_) != commandArgs.end();
}

inline void MidasProcessBase::setup_signal_handler() {
    struct sigaction act;
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGALRM, &act, NULL);

    sigemptyset(&sigWaitSet);
    sigaddset(&sigWaitSet, SIGHUP);
    sigaddset(&sigWaitSet, SIGINT);
    sigaddset(&sigWaitSet, SIGTERM);
    signalHandler = std::make_shared<boost::thread>(std::bind(&MidasProcessBase::on_signal, this));
}

inline void MidasProcessBase::on_signal() {
    int tid = ::syscall(__NR_gettid);
    int ret = 0;
    do {
        int sig = -1;
        if ((ret = sigwait(&sigWaitSet, &sig)) == 0) {
            MIDAS_LOG_INFO("received signal" << strsignal(sig) << "(" << sig << ") in thread " << tid);
            stop();
        }
    } while (ret);
}

inline string MidasProcessBase::config_key_from_cmd_arg_switch(const string& longSwitch_) {
    static string prefix("cmdline.");
    return prefix + longSwitch_;
}

inline void MidasProcessBase::init_config(int argc, char* argv[]) {
    string originalCmdLine;
    if (argc > 1) {
        originalCmdLine = argv[1];
    }
    for (int i = 2; i < argc; ++i) {
        originalCmdLine += " ";
        originalCmdLine += argv[i];
    }
    Config::instance().set<string>(config_key_from_cmd_arg_switch("original"), originalCmdLine);

    int count = commandArgs.size() + 1;
    struct option* pLongOptions = new struct option[count + 1];
    memset(pLongOptions, 0, (count + 1) * sizeof(struct option));
    struct option* pLongOptionPtr = pLongOptions;
    char shortOptionString[1024];
    char* shortOptionPtr = shortOptionString;

    *shortOptionPtr++ = 'h';
    *shortOptionPtr++ = '?';

    pLongOptionPtr->name = "help";
    pLongOptionPtr->has_arg = 0;
    pLongOptionPtr->flag = NULL;
    pLongOptionPtr->val = 'h';
    ++pLongOptionPtr;

    for (auto itr = commandArgs.begin(); itr != commandArgs.end(); ++itr) {
        *shortOptionPtr++ = (*itr).second->shortCmd;
        if ((*itr).second->isWithArg) {
            *shortOptionPtr++ = ':';
        }

        pLongOptionPtr->name = (*itr).second->longCmd.c_str();
        pLongOptionPtr->has_arg = (*itr).second->isWithArg ? 1 : 0;
        pLongOptionPtr->flag = NULL;
        pLongOptionPtr->val = (*itr).second->shortCmd;
        ++pLongOptionPtr;
    }

    *shortOptionPtr = '\0';

    Config tmpConfig;

    // parse process arguments
    int c = -1;
    int optionIndex = 0;
    while ((c = getopt_long(argc, argv, shortOptionString, pLongOptions, &optionIndex)) != -1) {
        if (c == 'h') usage();

        auto itr = commandArgs.find(c);
        if (itr != commandArgs.end()) {
            std::string configKey{config_key_from_cmd_arg_switch((*itr).second->longCmd)};
            if ((*itr).second->isWithArg) {
                string value(optarg);
                MIDAS_LOG_INFO("setting config arg " << configKey << " to " << value);
                tmpConfig.put<string>(configKey, value);
            } else {
                MIDAS_LOG_INFO("setting config arg " << configKey << " to true");
                tmpConfig.put<bool>(configKey, true);
            }
        }
    }

    delete[] pLongOptions;

    // read config if provided
    string configKey{config_key_from_cmd_arg_switch("config")};
    string configFiles = tmpConfig.get<string>(configKey, MidasConstants::instance().ConfigNullValue);
    if (configFiles != MidasConstants::instance().ConfigNullValue) {
        boost::char_separator<char> sep(",");
        boost::tokenizer<boost::char_separator<char>> token(configFiles, sep);

        for (auto itr = token.begin(); itr != token.end(); ++itr) {
            string configFile(*itr);
            try {
                Config::instance().read_config(configFile);
            } catch (const std::exception& e) {
                MIDAS_LOG_ERROR("unable to read config: " << configFile << " error: " << e.what());
                throw;
            }
        }
    }

    // override config with command line args
    Config::instance().merge_config(tmpConfig);

    // sanity check
    for (auto itr = commandArgs.begin(); itr != commandArgs.end(); ++itr) {
        string configKey{config_key_from_cmd_arg_switch((*itr).second->longCmd)};
        if (!(*itr).second->isOptional &&
            Config::instance().get<string>(configKey, MidasConstants::instance().ConfigNullValue) ==
                MidasConstants::instance().ConfigNullValue) {
            std::ostringstream os;
            os << "missing mandatory option -" << (*itr).second->shortCmd;
            usage(os.str());
        }
    }

    // identity config
    string procIdentityName(Config::instance().get<string>("identity.name"));
    char tmp[1024];
    string host("unknown");
    pid_t pid = getpid();
    if (gethostname(tmp, sizeof(tmp)) == 0) host = tmp;

    Config::instance().put("identity.host", host);
    Config::instance().put("identity.pid", pid);
    Config::instance().put("identity.executable", processName);

    if (procIdentityName.empty()) {
        std::ostringstream os;
        os << host << ":" << processName << ":" << pid;
        Config::instance().put("identity.name", os.str());
    }

    set_log_level(argc, argv);
}

inline void MidasProcessBase::_init_admin() {
    if (portal) {
        portal->register_admin("set log level", boost::bind(&MidasProcessBase::admin_set_log_level, this, _1, _2),
                               "get/set current log level", "option: ERROR WARNING INFO DEBUG");

        portal->register_admin("show config", boost::bind(&MidasProcessBase::admin_show_config, this, _1, _2),
                               "show config item", "show_config ALL | path");

        portal->register_admin("load config", boost::bind(&MidasProcessBase::admin_load_config, this, _1, _2),
                               "load config file and merge into global config", "load_config path");

        portal->register_admin("set config", boost::bind(&MidasProcessBase::admin_set_config, this, _1, _2),
                               "set value of a config item", "set_config key value");

        portal->register_admin("meters", boost::bind(&AdminCallbackManager::call, &metersManager, _1, _2),
                               "display metrics", "meters");

        portal->register_admin("shutdown", boost::bind(&MidasProcessBase::admin_shutdown, this, _1, _2),
                               "shutdown process", "shutdown");

        portal->register_admin("get env", boost::bind(&MidasProcessBase::admin_get_env, this, _1, _2),
                               "get/set environment variables", "getenv all|variable");
    }
}

inline void MidasProcessBase::start() {
    app_start();
    if (portal) {  // start admin
        if (!portal->start()) {
            usage();
        }
    }
    app_ready();
}

inline void MidasProcessBase::run() {
    std::unique_lock<std::mutex> lock(showdownMtx);
    while (!shutdown) {
        shutdownCondition.wait(lock);
    }

    if (portal) portal->stop();
}
inline void MidasProcessBase::stop() {
    app_stop();
    {
        std::lock_guard<std::mutex> lock(showdownMtx);
        shutdown = true;
        shutdownCondition.notify_all();
    }
}
inline string MidasProcessBase::admin_set_log_level(const string& cmd, const TAdminCallbackArgs& args) {
    string currentPriority = to_string(Log::instance().priority_m);
    ostringstream os;
    if (args.size() > 0) {
        string newPriority{args[0]};
        LogPriority priority = from_string_log_priority(newPriority.c_str());
        MIDAS_LOG_SET_PRIORITY(priority);
        os << "set log priority to " << newPriority;
    } else {
        os << "current log priority is " << currentPriority;
    }
    return os.str();
}
inline string MidasProcessBase::admin_show_config(const string& cmd, const TAdminCallbackArgs& args) {
    ostringstream os;
    if (args.size() == 0)
        return MIDAS_ADMIN_HELP_RESPONSE;
    else if (args[0] == "ALL")
        Config::instance().dump_ptree(os);
    else {
        string value = Config::instance().get<string>(args[0], "null");
        if (value != "null") {
            os << value;
        } else {
            os << "key " << args[0] << " does not exist.";
        }
    }
    return os.str();
}
inline string MidasProcessBase::admin_load_config(const string& cmd, const TAdminCallbackArgs& args) {
    ostringstream os;
    if (args.size() > 0) {
        string configFile{args[0]};
        try {
            Config::instance().read_config(configFile);
            os << "read config success.";
        } catch (const std::exception& e) {
            os << "unable to read config: " << configFile << " error: " << e.what();
            MIDAS_LOG_ERROR(os.str());
        }
    } else {
        return MIDAS_ADMIN_HELP_RESPONSE;
    }
    return os.str();
}
inline string MidasProcessBase::admin_set_config(const string& cmd, const TAdminCallbackArgs& args) {
    ostringstream os;
    if (args.size() < 2)
        return MIDAS_ADMIN_HELP_RESPONSE;
    else {
        string value = string_join(args);
        try {
            Config::instance().set<string>(args[0], value);
            os << "successfully set " << args[0] << " to " << value;
        } catch (const exception& e) {
            os << "failed to set " << args[0] << " to " << value;
            MIDAS_LOG_ERROR(os.str());
        }
    }
    return os.str();
}
inline string MidasProcessBase::admin_shutdown(const string& cmd, const TAdminCallbackArgs& args) {
    MIDAS_LOG_INFO("shutdown process due to shutdown admin command.");
    stop();
    return "";
}
inline string MidasProcessBase::admin_get_env(const string& cmd, const TAdminCallbackArgs& args) {
    ostringstream os;
    char* currentEnv = *environ;
    if (args.size() == 0) {
        for (int i = 1; currentEnv; ++i) {
            os << currentEnv << '\n';
            currentEnv = *(environ + i);
        }
    } else {
        bool found = false;
        size_t position;
        string currentVariable;
        for (int i = 1; currentEnv && !found; ++i) {
            currentVariable = currentEnv;
            if ((position = currentVariable.find("=")) != string::npos) {
                if (!strcasecmp(currentVariable.substr(0, position).c_str(), args[0].c_str())) {
                    os << currentVariable << '\n';
                    found = true;
                }
            }
            currentEnv = *(environ + i);
        }
        if (!found) os << args[0] << " not found" << '\n';
    }
    return os.str();
}

inline void MidasProcessBase::run_control_script() {
    string ctlFile = Config::instance().get<string>("cmdline.ctl_script");
    if (!ctlFile.empty()) {
        if (::access(ctlFile.c_str(), X_OK) != 0) {
            THROW_MIDAS_EXCEPTION("can not execute ctl script: \"" << ctlFile << "\"");
        }

        MIDAS_LOG_INFO("executing ctl script: \"" << ctlFile << "\"");
        int ret = ::system(ctlFile.c_str());
        int errorCode = ret >> 8;
        if (errorCode == 0) {
            MIDAS_LOG_INFO("executing ctl script: \"" << ctlFile << "\" successfully");
        } else {
            THROW_MIDAS_EXCEPTION("executing ctl script: \"" << ctlFile << "\" failed");
        }
    }
}

inline void MidasProcessBase::usage(string msg) {
    string usageText = usage_text();
    if (!msg.empty()) {
        cerr << msg << "\n";
    }
    cerr << usageText;
    exit(42);
}
inline string MidasProcessBase::usage_text() {
    ostringstream cmdLine, optHelp;
    vector<char> orderedArgs1, orderedArgs2;
    for (auto itr = commandArgs.begin(); itr != commandArgs.end(); ++itr) {
        if (!(*itr).second->isOptional)
            orderedArgs1.push_back((*itr).first);
        else
            orderedArgs2.push_back((*itr).first);
    }
    copy(orderedArgs2.begin(), orderedArgs2.end(), back_inserter(orderedArgs1));

    cmdLine << "Usage: " << processName << " \n";
    for (size_t i = 0; i < orderedArgs1.size(); ++i) {
        auto itr = commandArgs.find(orderedArgs1[i]);
        const CommandArgPtr ptr = (*itr).second;
        if (ptr->isOptional) cmdLine << "[ ";
        cmdLine << "-" << ptr->shortCmd << "|--" << ptr->longCmd << " ";
        if (ptr->isWithArg) cmdLine << "<" << ptr->argName << ">";
        if (ptr->isOptional) cmdLine << "]";
        cmdLine << ' ';
        optHelp << '\t' << "-" << ptr->shortCmd << "|--" << std::left << std::setw(20) << ptr->longCmd << " ";
        if (ptr->isOptional)
            cmdLine << "(optional) ";
        else
            cmdLine << "           ";

        size_t maxLine{120};
        vector<string> lines;
        text_word_warp(ptr->help, maxLine, lines);
        if (lines.size() > 0) {
            auto itr = lines.begin();
            optHelp << *(itr++) << '\n';
            for (; itr != lines.end(); ++itr) {
                optHelp << std::setw(45) << ' ' << *itr << '\n';
            }
        } else {
            optHelp << '\n';
        }
    }
    return string(cmdLine.str() + optHelp.str());
}

inline void MidasProcessBase::set_log_level(int argc, char** argv) const {
    string opt = Config::instance().get<string>("cmd.loglevel");
    if (opt.empty()) {
        // try to find -L info or --loglevel info
        bool isLevel = false;
        for (auto pos = 1; pos < argc; ++pos) {
            const auto arg = argv[pos];
            if (isLevel) {
                opt = arg;
                break;
            }

            if ('-' == arg[0]) {
                if (!strcmp("L", arg + 1))
                    isLevel = true;
                else if ('-' == arg[1] && !strcmp("loglevel", arg + 2)) {
                    isLevel = true;
                }
            }
        }
    }

    if (opt.empty()) return;

    LogPriority newLevel;
    if (boost::iequals(MIDAS_LOG_PRIORITY_STRING_ERROR, opt)) {
        newLevel = ERROR;
    } else if (boost::iequals(MIDAS_LOG_PRIORITY_STRING_WARNING, opt)) {
        newLevel = WARNING;
    } else if (boost::iequals(MIDAS_LOG_PRIORITY_STRING_INFO, opt)) {
        newLevel = INFO;
    } else if (boost::iequals(MIDAS_LOG_PRIORITY_STRING_DEBUG, opt)) {
        newLevel = DEBUG;
    } else {
        MIDAS_LOG_WARNING("ignoring unknown value cmd.loglevel :" << opt);
        return;
    }

    MIDAS_LOG_SET_PRIORITY(newLevel);
}
}

#endif
