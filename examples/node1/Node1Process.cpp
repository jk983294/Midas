#include "Node1Process.h"
#include "net/buffer/MfBuffer.h"

Node1Process::Node1Process(int argc, char** argv) : MidasProcessBase(argc, argv), channelOut("output") {
    init_admin();
    register_command_line_args('H', "heart_beat_interval", SwitchArgType::SwitchWithArg,
                               SwitchOptionType::SwitchOptional, "heart beat interval", "heart beat interval (second)");
    timer.pause();
    timer.set_callback([this] { heart_beat_timer(); });
}

Node1Process::~Node1Process() { timer.shutdown(); }

void Node1Process::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }

    TTcpOutputAcceptor::new_instance(dataPort, channelOut);
    channelOut.start();

    configure_heart_beats();
    timer.resume();
}

void Node1Process::app_stop() { channelOut.stop(); }

uint32_t Node1Process::configure_heart_beats() {
    // default 5 second a heart beat
    uint32_t interval = Config::instance().get<uint32_t>("cmdline.heart_beat_interval", 5);
    timer.set_interval(interval);
    return interval;
}

void Node1Process::heart_beat_timer() {
    time_t now;
    std::time(&now);

    // give downstream heartbeat
    MfBuffer mf;
    mf.start("hbt");
    mf.add_timestamp("send_time", now, 0);
    mf.finish();

    channelOut.deliver(ConstBuffer(mf.data(), mf.size()));
}

void Node1Process::init_admin() {
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
