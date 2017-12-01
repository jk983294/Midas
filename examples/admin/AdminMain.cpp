#include <utils/MidasUtils.h>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "net/common/NetworkHelper.h"

using boost::asio::ip::tcp;
using namespace boost::property_tree;
using namespace std;
using namespace midas;

/**
 * tcp based, send admin cmd then get response from server
 * protocol is based on json, the delimiter '}' works as eof
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "usage: admin host:port[:interface] cmd [parameter]" << '\n';
        return -1;
    }

    string hostPort{argv[1]}, cmd{argv[2]};
    ostringstream ss;
    for (int i = 3; i < argc;) {
        if (argv[i][0] == '-') {
            ss << '"' << argv[i++];
            if (i < argc) {
                if (argv[i][0] != '-') {
                    ss << ' ' << argv[i++];
                }
            }
        } else {
            ss << '"' << argv[i++];
        }
        ss << "\",";
    }
    string arguments = ss.str();
    if (arguments.size()) {
        arguments.erase(arguments.size() - 1);
    }

    string host, port, interface;
    midas::split_host_port_interface(hostPort, host, port, interface);

    ss.str("");  // clear content, ss.clear() only clear error flag
    ss << "{"
       << "\"userId\":\"" << get_user_id() << "\","
       << "\"command\":\"" << cmd << "\","
       << "\"requestId\":\"" << 1 << "\","
       << "\"arguments\":[" << arguments << "]"
       << "}";

    string request{ss.str()};

    try {
        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query(host, port);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        tcp::socket socket(io_service);
        // try each of resolved iterator until we find one that works
        boost::asio::connect(socket, endpoint_iterator);

        boost::asio::write(socket, boost::asio::buffer(request, request.length()));

        ostringstream oss;

        for (;;) {
            boost::array<char, 1024> buf;
            boost::system::error_code error;

            size_t len = socket.read_some(boost::asio::buffer(buf), error);

            if (error == boost::asio::error::eof)
                break;  // Connection closed cleanly by peer.
            else if (error)
                throw boost::system::system_error(error);

            oss.write(buf.data(), len);

            if (len > 0 && buf[len - 1] == '}') {
                break;
            }
        }

        ptree pt;
        stringstream jsonString(oss.str());
        read_json(jsonString, pt);
        string userId = pt.get<string>("userId");
        int requestId = pt.get<int>("requestId");
        string response = pt.get<string>("response");
        std::cout << response << '\n';
    } catch (std::exception& e) {
        std::cerr << "error connect to server " << host << ":" << port << " with parameter: " << arguments << '\n'
                  << "reason: " << e.what() << '\n';
    }
    return 0;
}
