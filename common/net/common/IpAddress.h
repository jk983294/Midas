#ifndef MIDAS_IP_ADDRESS_H
#define MIDAS_IP_ADDRESS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "midas/MidasConfig.h"
#include "utils/ConvertHelper.h"

using namespace std;

namespace midas {

class IpAddress {
public:
    string ip;
    in_addr inAddr;
    string host;
    vector<string> aliasList;

public:
    IpAddress() { memset(&inAddr, '\0', sizeof(inAddr)); }
    IpAddress(const string& hOrIp) { set_from_host_or_ip(hOrIp); }
    IpAddress(const IpAddress& other) { copy(other); }
    IpAddress& operator=(const IpAddress& other) {
        copy(other);
        return *this;
    }
    bool operator==(const IpAddress& other) const { return compare(other) == 0; }
    bool operator!=(const IpAddress& other) const { return compare(other) != 0; }
    bool is_valid() const { return inAddr.s_addr > 0; }

    void set_from_host_or_ip(const string& hOrIp) {
        char first = hOrIp[0];
        if (::isdigit(first))
            set_from_ip_address(hOrIp);
        else
            set_from_host(hOrIp);
    }

    friend ostream& operator<<(ostream& os, const IpAddress& ipAddress) {
        os << ipAddress.ip;
        return os;
    }

protected:
    void copy(const IpAddress& other) {
        ip = other.ip;
        memcpy(&inAddr, &other.inAddr, sizeof(inAddr));
        host = other.host;
        aliasList = other.aliasList;
    }

    int compare(const IpAddress& other) const {
        if (inAddr.s_addr == other.inAddr.s_addr)
            return 0;
        else if (inAddr.s_addr < other.inAddr.s_addr)
            return -1;
        else if (inAddr.s_addr > other.inAddr.s_addr)
            return 1;
        return 0;
    }

private:
    void set_from_ip_address(const string& ipAddress) {
        copy(IpAddress());
        if (inet_aton(ipAddress.c_str(), &inAddr)) {
#ifdef SOLARIS
            struct hostent* pHostEnt = ::gethostbyaddr((char*)&inAddr.s_addr, sizeof(inAddr.s_addr), AF_INET);
#else
            struct hostent* pHostEnt = ::gethostbyaddr(&inAddr, sizeof(inAddr), AF_INET);
#endif
            if (pHostEnt) {
                extract_from_hostent(pHostEnt);
            } else {
                ip = ipAddress;
            }
        }
    }
    void set_from_host(const string& hostName) {
        copy(IpAddress());
        struct hostent* pHostEnt = ::gethostbyname(hostName.c_str());
        extract_from_hostent(pHostEnt);
    }
    void extract_from_hostent(const hostent* pHostent) {
        if (pHostent) {
            host = pHostent->h_name;
            int maxAlias = 10;
            for (int i = 0; i < maxAlias; ++i) {
                char* pAlias = pHostent->h_aliases[i];
                if (pAlias != NULL) {
                    string sAlias(pAlias);
                    aliasList.push_back(sAlias);

                    if (i == 0 && sAlias.find(host) == 0 && sAlias.find("-") == host.size()) {
                        host = sAlias;
                    }
                } else {
                    break;
                }
            }
            if (pHostent->h_addrtype == AF_INET) {
                inAddr.s_addr = *(uint32_t*)(pHostent->h_addr_list[0]);
                ip = inet_ntoa(inAddr);
            }
        } else {
            copy(IpAddress());
        }
    }
};

class TcpAddress {
public:
    std::string host;
    std::string service;
    std::string itf;
    std::string cfg;

public:
    TcpAddress(const string& h, const string& s, const string& i = string(), const string& c = string())
        : host(h),
          service(s),
          itf(i.empty() ? Config::instance().get<string>(c + "bind_address", string()) : i),
          cfg(c) {
        assert(!host.empty() && !service.empty());
    }

    TcpAddress(const string& h, unsigned short port, const string& i = string(), const string& c = string())
        : host(h), itf(i.empty() ? Config::instance().get<string>(c + "bind_address", string()) : i), cfg(c) {
        service = int2str(port);
        assert(!host.empty() && !service.empty());
    }

    template <typename Stream>
    void print(Stream& s) const {
        s << " tcp address[ host[" << host << "] service[" << service << "] itf[" << itf << "] cfg[" << cfg << "]";
    }

    TcpAddress() {}
};

template <typename Stream>
inline Stream& operator<<(Stream& s, const TcpAddress& addr) {
    addr.print(s);
    return s;
}

struct UdpAddress {
    string host;
    string service;
    string interface;
    string ip;
    string bindIp;
    bool multicast{false};  // decided in resolve phase, if ip is D class, then it is multicast
    unsigned short port{0};
    unsigned short bindPort{0};
    UdpAddress() {}
    UdpAddress(const string& h, const string& s, const string& i) : host(h), service(s), interface(i) {}
};
}

#endif
