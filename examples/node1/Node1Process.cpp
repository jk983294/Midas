#include "Node1Process.h"
#include "net/tcp/TcpAcceptor.h"
#include "net/tcp/TcpPublisherAsync.h"
#include "process/admin/MidasAdminBase.h"
#include "utils/log/Log.h"

struct TcpOutput {};

typedef TcpAcceptor<TcpPublisherAsync<TcpOutput>> TTcpOutputAcceptor;

Node1Process::Node1Process(int argc, char** argv)
    : MidasProcessBase(argc, argv), channelIn("input"), channelOut("output") {
    register_command_line_args('L', "loglevel", SwitchWithArg, SwitchOptional, "set log level", "set log level");
    set_log_level(argc, argv);
    init_admin();
    timer.pause();
    timer.set_callback([this] { heart_beat_timer(); });
}

Node1Process::~Node1Process() {
    cancelAdmin = true;
    cvAdmin.notify_all();
    timer.shutdown();
}

void Node1Process::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }

    const string port = Config::instance().get<string>("cmd.server_port", "8023");
    TTcpOutputAcceptor::new_instance(port, channelOut);
    channelOut.start();
}

void Node1Process::app_stop() { channelOut.stop(); }

uint32_t Node1Process::configure_heart_beats(bool lock) {
    // default 5 second a heart beat
    uint32_t interval = Config::instance().get<uint32_t>("cmd.heart_beat_interval", 5);
    timer.set_interval(interval);
    return interval;
}

void Node1Process::set_log_level(int argc, char** argv) const {
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
    if (MIDAS_LOG_PRIORITY_STRING_ERROR == opt) {
        newLevel = ERROR;
    } else if (MIDAS_LOG_PRIORITY_STRING_WARNING == opt) {
        newLevel = WARNING;
    } else if (MIDAS_LOG_PRIORITY_STRING_INFO == opt) {
        newLevel = INFO;
    } else if (MIDAS_LOG_PRIORITY_STRING_DEBUG == opt) {
        newLevel = DEBUG;
    } else {
        MIDAS_LOG_WARNING("ignoring unknown value cmd.loglevel :" << opt);
        return;
    }

    MIDAS_LOG_SET_PRIORITY(newLevel);
}

void Node1Process::heart_beat_timer() {
    time_t now;
    time(&now);
    // give downstream heartbeat
}

void Node1Process::init_admin() {
    TTcpReceiver::register_admin(channelIn, admin_handler());
    admin_handler().register_callback("meters", boost::bind(&Node1Process::admin_meters, this, _1, _2),
                                      "display statistical information of connections", "meters");
}

string Node1Process::admin_meters(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    oss << "clients:\n";
    channelOut.meters(oss, true);
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
