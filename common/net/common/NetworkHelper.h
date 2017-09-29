#ifndef MIDAS_NETWORK_HELPER_H
#define MIDAS_NETWORK_HELPER_H

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include "midas/MidasException.h"
#include "net/common/IpAddress.h"

#ifdef SOLARIS
#include <sys/sockio.h>
#endif

using namespace std;

namespace midas {

inline void split_host_port_interface(const string& str, string& host, string& port, string& itf) {
    boost::tokenizer<boost::char_separator<char>> token(str, boost::char_separator<char>(":"));
    boost::tokenizer<boost::char_separator<char>>::iterator it = token.begin();
    if (it != token.end()) host = boost::trim_copy(*it++);
    if (it != token.end()) port = boost::trim_copy(*it++);
    if (it != token.end()) itf = boost::trim_copy(*it);
    if (port.empty()) {
        port = host;
        host = "localhost";
    }
}

inline void split_host_port_interface(const string& str, string& host, uint16_t& port, string& itf) {
    string portStr;
    split_host_port_interface(str, host, portStr, itf);
    char first = portStr[0];
    if (isdigit(first)) {
        port = atoi(portStr.c_str());
    }
}

class NetworkHelper {
public:
    typedef boost::interprocess::interprocess_mutex MyMutex;
    typedef boost::interprocess::scoped_lock<MyMutex> MyMutexLock;
    const int MAX_INTERFACES = 20;

    map<string, string> interface2address;
    MyMutex mtx;

public:
    /**
     * given eth0 or AAA.BBB.CCC.DDD, find actual ip address to interfaceAddress
     */
    void lookup_interface(const string& interfaceOrSubnet, string& interfaceAddress) {
        download_interface_map();
        MyMutexLock(mtx);
        string interface{interfaceOrSubnet};

        // firstly check if it is a subnet
        if (::isdigit(interface[0])) {
            string subnetStart;
            int bits = 0;
            parse_subnet_and_bits(interface, subnetStart, bits);
            if (bits > 0) {
                lookup_interface_from_subnet_no_lock(subnetStart, bits, interface);
            }
        }

        auto itr = interface2address.find(interface);
        if (itr == interface2address.end()) {
            stringstream ss;
            ss << "interface " << interface << " is not found in this host's interface list.";
            throw MidasException(ss.str(), __FILE__, __LINE__);
        } else {
            interfaceAddress = itr->second;
        }
    }

    /**
     * given eth0 or AAA.BBB.CCC.DDD, find actual ip address to interfaceAddress
     */
    void lookup_interface(const string& interfaceOrSubnet, IpAddress& interfaceAddress) {
        string sInterfaceAddress;
        lookup_interface(interfaceOrSubnet, sInterfaceAddress);
        IpAddress tmp(sInterfaceAddress);
        interfaceAddress = tmp;
    }

    /**
     * parse AAA.BBB.CCC.DDD/bits (like 192.168.0.128/25)
     * @param subnet to parse
     * @param sSubnet return subnet part
     * @param bits return bits part (0 error, default 32)
     */
    static void parse_subnet_and_bits(const string& subnet, string& sSubnet, int& bits) {
        boost::char_separator<char> sep("/");
        boost::tokenizer<boost::char_separator<char>> token(subnet, sep);
        auto itr = token.begin();
        if (itr != token.end()) {
            sSubnet = *itr++;
        } else {
            sSubnet = subnet;
            bits = 0;
        }

        if (itr != token.end()) {
            bits = atoi(itr->c_str());
        } else {
            bits = 32;
        }
    }

    /**
     * given subnet definition AAA.BBB.CCC.DDD/bits, find interface that belongs to subnetAddress
     * @param subnetAddress to look for
     * @param bits
     * @param interfaceAddress out parameter like (eth0)
     */
    void lookup_interface_from_subnet(const string& subnetAddress, int bits, string& interfaceAddress) {
        download_interface_map();
        MyMutexLock(mtx);
        lookup_interface_from_subnet_no_lock(subnetAddress, bits, interfaceAddress);
    }

    bool is_address_in_subnet(const string& ip, const string& lookupSubnetAddress, int& bits) {
        if (bits < 1 || bits > 32) return false;
        in_addr ipAddr;
        if (!::inet_aton(ip.data(), &ipAddr)) return false;

        unsigned hostBits = 32 - bits;
        unsigned subnetSize = (unsigned)std::pow(2.0, (double)hostBits);
        in_addr subnetAddr;
        subnetAddr.s_addr = htonl((unsigned(ntohl(ipAddr.s_addr) / subnetSize)) * subnetSize);
        string sSubnetAddr(::inet_ntoa(subnetAddr));
        if (sSubnetAddr == lookupSubnetAddress) return true;
        return false;
    }

protected:
    void download_interface_map() {
        MyMutexLock(mtx);

        if (interface2address.empty()) {
            ifconf interfaceConf;
            ifreq interfaceReqBuffer[MAX_INTERFACES];
            memset(&interfaceConf, '\0', sizeof(interfaceConf));
            memset(&interfaceReqBuffer, '\0', sizeof(interfaceReqBuffer));

            int socket = 0;
            if ((socket = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                stringstream ss;
                ss << "error create socket failed: " << errno << " - " << strerror(errno);
                throw MidasException(ss.str(), __FILE__, __LINE__);
            }

            interfaceConf.ifc_len = sizeof(interfaceReqBuffer);
            interfaceConf.ifc_buf = (char*)interfaceReqBuffer;

            if (::ioctl(socket, SIOCGIFCONF, &interfaceConf) < 0) {
                stringstream ss;
                ss << "error ioctl SIOCGIFCONF failed: " << errno << " - " << strerror(errno);
                throw MidasException(ss.str(), __FILE__, __LINE__);
            }

            for (int i = 0; i < MAX_INTERFACES && interfaceReqBuffer[i].ifr_name[0] != '\0'; ++i) {
                in_addr* pInAddr = &(((sockaddr_in*)&interfaceReqBuffer[i].ifr_addr)->sin_addr);
                if (pInAddr->s_addr > 0) {
                    const char* interfaceIp = ::inet_ntoa(*pInAddr);
                    string name(interfaceReqBuffer[i].ifr_name);
                    string ip(interfaceIp);
                    interface2address.insert(map<string, string>::value_type(name, ip));
                }
            }

            ::close(socket);
        }
    }

    void lookup_interface_from_subnet_no_lock(const string& subnetAddress, int& bits, string& interfaceName) {
        for (auto itr = interface2address.begin(); itr != interface2address.end(); ++itr) {
            string interfaceIp{itr->second};
            if (is_address_in_subnet(interfaceIp, subnetAddress, bits)) {
                interfaceName = itr->first;
                return;
            }
        }

        stringstream ss;
        ss << "no interface in host's list belong to subnet : " << subnetAddress << "/" << bits;
        throw MidasException(ss.str(), __FILE__, __LINE__);
    }
};
}

#endif
