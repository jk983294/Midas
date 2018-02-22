#include <midas/MidasConfig.h>
#include <midas/md/MdDefs.h>
#include <net/raw/MdProtocol.h>
#include <net/raw/RawSocket.h>
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <utils/log/Log.h>
#include "PubControlChannel.h"
#include "Publisher.h"

using namespace std;
using namespace midas;

namespace {
thread_local bool control_thread = false;
}

PubControlChannel::PubControlChannel(std::string const& key, Publisher* publisher_) : publisher(publisher_) {
    ipPublisher = midas::get_cfg_value<string>(key, "publisher_ip");
    portPublisher = midas::get_cfg_value<uint16_t>(key, "publisher_port");
    intervalHeartbeat = midas::get_cfg_value<uint32_t>(key, "heartbeat_interval", 1);
    maxMissingHB = midas::get_cfg_value<uint32_t>(key, "max_missing_heartbeat_interval", 5);
    intervalKeepTicking = midas::get_cfg_value<uint32_t>(key, "house_keeping_interval", 20);
    controlCore = midas::get_cfg_value<string>(key, "control_core");

    if ((20 > intervalKeepTicking) || (1000 < intervalKeepTicking)) {
        intervalKeepTicking = 20;
    }
    MIDAS_LOG_INFO(ipPublisher << ":" << portPublisher << " heartbeat_interval: " << intervalHeartbeat
                               << " house_keeping_interval: " << intervalKeepTicking
                               << " control_core: " << controlCore);
}

PubControlChannel::~PubControlChannel() { MIDAS_LOG_INFO("Destroying control channel"); }

/**
 * reading packetCount message packets where one message packet => [1xHeader + nxMsgs]
 * @param sockFd
 * @param packetCount continue reading if packetCount is 0
 * @return
 */
int PubControlChannel::receive(int sockFd, int packetCount) {
    uint8_t buffer[1024];
    int pktsRcvd = 0;
    bool cleanup = false;

    for (;;) {
        Header hdr;
        ssize_t n = read_socket(sockFd, (uint8_t*)&hdr, sizeof(Header));  // Reader Header first
        if (n > 0) {
            if (hdr.size == 0) {
                // heartbeat
            } else {
                n = read_socket(sockFd, buffer, hdr.size);
                if (n > 0) {
                    bool isConnected = false;
                    uint8_t* datap = buffer;
                    for (auto c = 0; c < hdr.count; ++c) {
                        switch (*datap) {
                            case CTRL_CONNECT_TYPE:
                                MIDAS_LOG_INFO("Dispatching onConnectV2");
                                publisher->on_connect(sockFd, &hdr, (CtrlConnect*)datap);
                                break;
                            case CTRL_DISCONNECT_TYPE:
                                MIDAS_LOG_INFO("Dispatching on_disconnect");
                                publisher->on_disconnect(sockFd, &hdr, (CtrlDisconnect*)datap);
                                break;
                            case CTRL_SUBSCRIBE_TYPE:
                                publisher->on_subscribe(sockFd, &hdr, (CtrlSubscribe*)datap);
                                break;
                            case CTRL_UNSUBSCRIBE_TYPE:
                                publisher->on_unsubscribe(sockFd, &hdr, (CtrlUnsubscribe*)datap);
                                break;
                            default:
                                break;
                        }
                        datap += 1;             // advance by 1 byte to size field
                        datap += (*datap - 1);  // advance to next message
                    }
                    // multiple CtrlConnectResponse is delivered in message unit
                    // call signal_connected only when the entire unit is processed
                    //
                    if (isConnected) {
                        signal_connected();
                    }
                } else {
                    cleanup = true;
                    break;
                }
            }
            if (++pktsRcvd == packetCount) {  // won't ever break if packetCount == 0
                break;
            }
        } else {
            cleanup = true;
            break;
        }
    }

    if (cleanup) {
        close_socket(sockFd);
    }
    return (cleanup ? -1 : pktsRcvd);
}

bool PubControlChannel::producer_server() {
    if (fdControl == -1) {
        if ((fdControl = make_tcp_socket_server(ipPublisher.c_str(), portPublisher)) >= 0) {
            MIDAS_LOG_INFO("producer accepting connection on " << ipPublisher << ":" << portPublisher);
            epoll_event inEvent;
            inEvent.events = EPOLLIN;
            inEvent.data.fd = fdControl;
            if (-1 == epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fdControl, &inEvent)) {
                MIDAS_LOG_ERROR("epoll_ctl failed: " << errno);
                close_socket(fdControl);
                fdControl = -1;
            }
        }
    }
    return fdControl >= 0;
}

void PubControlChannel::signal_connected() {
    MIDAS_LOG_INFO("Handshake with producer completed");
    pthread_mutex_lock(&connectedLock);
    connected = true;
    pthread_mutex_unlock(&connectedLock);
    pthread_cond_signal(&connectedCV);
}

void PubControlChannel::run_loop() {
    int nfds;
    epoll_event inEvent;
    epoll_event outEvent[128];

    if ((fdEpoll = epoll_create(1)) < 0) {
        MIDAS_LOG_ERROR("Failed on epoll_create " << errno << " " << strerror(errno));
        return;
    }

    inEvent.events = EPOLLIN;

    if (create_timer(intervalHeartbeat * 1000, &fdHeartbeatTimer)) {
        inEvent.data.fd = fdHeartbeatTimer;
        epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fdHeartbeatTimer, &inEvent);
    }

    if (create_timer(intervalKeepTicking /* ms */, &fdKeepTickingTimer)) {
        inEvent.data.fd = fdKeepTickingTimer;
        epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fdKeepTickingTimer, &inEvent);
    }

    while (!stopped) {
        producer_server();  // Establish server socket

        nfds = epoll_wait(fdEpoll, outEvent, sizeof(outEvent) / sizeof(outEvent[0]), 1);
        if (nfds == -1) {
            MIDAS_LOG_ERROR("ERROR: epoll_wait  " << strerror(errno));
            continue;
        }
        for (int n = 0; n < nfds; ++n) {
            int fdReady = outEvent[n].data.fd;
            if (fdReady == fdControl) {
                uint16_t portClient = 0;
                char* ipClient = nullptr;
                int fdClient = accept_tcp_socket_client(fdControl, &portClient, &ipClient);
                if (fdClient >= 0) {
                    MIDAS_LOG_WARNING("Accepted connection(" << fdClient << ") from consumer at " << ipClient << ":"
                                                             << portClient);
                    publisher->register_feed_client(fdClient, ipClient, portClient);
                    if (ipClient) {
                        free(ipClient);
                    }
                    inEvent.data.fd = fdClient;
                    epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fdClient, &inEvent);
                }
            } else if (fdReady == fdHeartbeatTimer) {
                uint64_t dummy;
                read(fdReady, &dummy, sizeof(dummy));
            } else if (fdReady == fdKeepTickingTimer) {
                uint64_t dummy;
                read(fdReady, &dummy, sizeof(dummy));
                publisher->on_house_keeping();
            } else {
                // Message from an consumer, receive one pkt at a time - there may be more than one msg in a pkt
                int packetCount = this->receive(fdReady, 1);
                if (packetCount < 0) {  // Socket would have been closed by receive and removed from epoll

                    MIDAS_LOG_WARNING("Consumer(" << fdReady << ") hung up?");
                    publisher->unregister_feed_client(fdReady);
                } else {
                    publisher->update_client_heartbeat(fdReady);
                }
            }
        }
    }
    destroy_timer(&fdHeartbeatTimer);
}

void* PubControlChannel::run(void* arg) {
    PubControlChannel* cc = static_cast<PubControlChannel*>(arg);
    MIDAS_LOG_INFO("Starting control channel thread, id: " << midas::get_tid());
    control_thread = true;
    cc->run_loop();  // blocks here
    return nullptr;
}
