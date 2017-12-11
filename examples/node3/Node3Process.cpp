#include "Node3Process.h"
#include "net/buffer/MfBuffer.h"

static const std::string id{"id"};

Node3Process::Node3Process(int argc, char** argv) : MidasProcessBase(argc, argv), channelIn("input") {
    init_admin();
    TTcpReceiver::data_callback(boost::bind(&Node3Process::data_notify, this, _1, _2));
}

Node3Process::~Node3Process() {}

void Node3Process::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }

    TTcpReceiver::new_instance("localhost", serverPort, channelIn);
}

void Node3Process::app_stop() { channelIn.stop(); }

size_t Node3Process::data_notify(const ConstBuffer& data, MemberPtr member) {
    size_t nMessages(0);
    size_t nBytesUsed = MidasMessageDeFrame::deframe_packet(data, bufferList, nMessages);
    ConstBufferSequence::const_iterator iterEnd = bufferList.end();

    for (ConstBufferSequence::const_iterator iter = bufferList.begin(); iter != iterEnd; ++iter) {
        std::vector<std::string> messages;
        split_messages(std::string(iter->data(), iter->size()), messages);

        for (auto& msg : messages) {
            msg.append(midas::constants::mfDelimiter);
            cout << msg << endl;
        }
    }

    return nBytesUsed;
}

void Node3Process::split_messages(const std::string& s, std::vector<std::string>& ret) {
    size_t last = 0;
    size_t length = s.size() - sizeof(MidasHeader);

    std::string str1(s.data() + sizeof(MidasHeader), length);
    size_t index = str1.find_first_of(midas::constants::mfDelimiter, last);

    while (index != std::string::npos) {
        ret.push_back(str1.substr(last, index - last));

        last = index + 1;
        index = str1.find_first_of(midas::constants::mfDelimiter, last);
    }
}

void Node3Process::init_admin() {
    // register open/kill commands for upstream connections
    TTcpReceiver::register_admin(channelIn, admin_handler());

    admin_handler().register_callback("meters", boost::bind(&Node3Process::admin_meters, this, _1, _2),
                                      "display statistical information of connections", "meters");
}

string Node3Process::admin_meters(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    oss << "Servers:\n";
    channelIn.meters(oss, true);
    return oss.str();
}

bool Node3Process::configure() {
    if (adminPort.empty()) {
        MIDAS_LOG_INFO("must provide admin port!");
        return false;
    }
    if (serverPort.empty()) {
        MIDAS_LOG_INFO("must provide server port!");
        return false;
    }
    return true;
}
