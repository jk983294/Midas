#include "Node2Process.h"
#include "net/buffer/MfBuffer.h"

static const std::string id{"id"};

Node2Process::Node2Process(int argc, char** argv)
    : MidasProcessBase(argc, argv), channelIn("input"), channelOut("output") {
    init_admin();
    TTcpReceiver::data_callback(boost::bind(&Node2Process::data_notify, this, _1, _2));
}

Node2Process::~Node2Process() {}

void Node2Process::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }

    TTcpReceiver::new_instance("localhost", serverPort, channelIn);

    TTcpOutputAcceptor::new_instance(dataPort, channelOut);
    channelOut.start();
}

void Node2Process::app_stop() {
    channelIn.stop();
    channelOut.stop();
}

size_t Node2Process::data_notify(const ConstBuffer& data, MemberPtr member) {
    size_t nMessages(0);
    size_t nBytesUsed = MidasMessageDeFrame::deframe_packet(data, bufferList, nMessages);
    ConstBufferSequence::const_iterator iterEnd = bufferList.end();

    for (ConstBufferSequence::const_iterator iter = bufferList.begin(); iter != iterEnd; ++iter) {
        std::vector<std::string> messages;
        split_messages(std::string(iter->data(), iter->size()), messages);

        for (auto& msg : messages) {
            msg.append(midas::constants::mfDelimiter);

            if (tick.parse(msg.c_str(), msg.size())) {
                if (tick.get("id").has_value()) {
                    MfBuffer outputBuffer;

                    outputBuffer.start(tick.get("id").value_string());

                    for (unsigned int iIndex = 0; iIndex < tick.size(); iIndex++) {
                        if (tick.at(iIndex).key_string() == id) {
                            continue;
                        } else {
                            outputBuffer.add_string(tick.at(iIndex).key_string(), tick.at(iIndex).value_string());
                        }
                    }

                    outputBuffer.finish();

                    MutableBuffer outBuffer(outputBuffer.size());
                    outBuffer.store(outputBuffer.data(), outputBuffer.size());
                    channelOut.deliver(outBuffer);
                }
            }
        }
    }

    return nBytesUsed;
}

void Node2Process::split_messages(const std::string& s, std::vector<std::string>& ret) {
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

void Node2Process::init_admin() {
    // register open/kill commands for upstream connections
    TTcpReceiver::register_admin(channelIn, admin_handler());

    admin_handler().register_callback("meters", boost::bind(&Node2Process::admin_meters, this, _1, _2),
                                      "display statistical information of connections", "meters");
}

string Node2Process::admin_meters(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    oss << "Servers:\n";
    channelIn.meters(oss, true);
    oss << '\n';
    oss << "clients:\n";
    channelOut.meters(oss, true);
    return oss.str();
}

bool Node2Process::configure() {
    if (adminPort.empty()) {
        MIDAS_LOG_INFO("must provide admin port!");
        return false;
    }
    if (dataPort.empty()) {
        MIDAS_LOG_INFO("must provide data port!");
        return false;
    }
    if (serverPort.empty()) {
        MIDAS_LOG_INFO("must provide server port!");
        return false;
    }
    return true;
}
