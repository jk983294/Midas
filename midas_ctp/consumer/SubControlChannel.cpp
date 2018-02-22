#include <midas/MidasConfig.h>
#include <midas/md/MdDefs.h>
#include <net/raw/MdProtocol.h>
#include <net/raw/RawSocket.h>
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <utils/log/Log.h>
#include "SubControlChannel.h"
#include "Subscriber.h"

using namespace std;
using namespace midas;

namespace {
thread_local bool control_thread = false;
}

PubControlChannel::PubControlChannel(std::string const& key, Publisher* feedServer_) : feedServer(feedServer_) {
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

PubControlChannel::~PubControlChannel() {
    MIDAS_LOG_INFO("Destroying control channel");

    if (mode == PubControlChannel::consumer) {
        pthread_cond_destroy(&connectedCV);
        pthread_mutex_destroy(&connectedLock);
    }
}

/**
 * reading 'npkts' message packets where one message packet => [1xHeader + nxMsgs]
 * @param sockfd
 * @param npkts continue reading if 'npkts' is 0
 * @return
 */
int PubControlChannel::recv(int sockfd, int npkts) {
    uint8_t buffer[1024];
    int pktsRcvd = 0;
    bool cleanup = false;

    for (;;) {
        Header hdr;
        ssize_t n = read_socket(sockfd, (uint8_t*)&hdr, sizeof(Header));  // Reader Header first
        if (n > 0) {
            if (hdr.size == 0) {
                // heartbeat
            } else {
                n = read_socket(sockfd, buffer, hdr.size);
                if (n > 0) {
                    bool isConnected = false;
                    uint8_t* datap = buffer;
                    for (auto c = 0; c < hdr.count; ++c) {
                        switch (*datap) {
                            case CTRL_CONNECT_TYPE:
                                MIDAS_LOG_INFO("Dispatching onConnectV2");
                                feedServer->on_connect(sockfd, &hdr, (CtrlConnect*)datap);
                                break;
                            case CTRL_CONNECT_RESPONSE_TYPE:
                                MIDAS_LOG_INFO("Dispatching onConnectResponse");
                                feedClient->onConnectResponse(&hdr, (CtrlConnectResponse*)datap, &isConnected);
                                break;
                            case CTRL_DISCONNECT_TYPE:
                                MIDAS_LOG_INFO("Dispatching onDisconnect");
                                feedServer->on_disconnect(sockfd, &hdr, (CtrlDisconnect*)datap);
                                break;
                            case CTRL_DISCONNECT_RESPONSE_TYPE:
                                MIDAS_LOG_INFO("Dispatching onDisconnectResponse");
                                feedClient->onDisconnectResponse(&hdr, (CtrlDisconnectResponse*)datap);
                                break;
                            case CTRL_SUBSCRIBE_TYPE:
                                feedServer->on_subscribe(sockfd, &hdr, (CtrlSubscribe*)datap);
                                break;
                            case CTRL_SUBSCRIBE_RESPONSE_TYPE:
                                feedClient->onSubscribeResponse(&hdr, (CtrlSubscribeResponse*)datap);
                                break;
                            case CTRL_UNSUBSCRIBE_TYPE:
                                feedServer->on_unsubscribe(sockfd, &hdr, (CtrlUnsubscribe*)datap);
                                break;
                            case CTRL_UNSUBSCRIBE_RESPONSE_TYPE:
                                feedClient->onUnsubscribeResponse(&hdr, (CtrlUnsubscribeResponse*)datap);
                                break;
                            default:
                                break;
                        }
                        datap += 1;             // advance by 1 byte to size field
                        datap += (*datap - 1);  // advance to next message
                    }
                    //
                    // Multiple CtrlConnectResponse is delivered in message unit
                    // Call 'signalConnected' only when the entire unit is processed
                    //
                    if (isConnected) {
                        signalConnected();
                        feedClient->onControlChannelEstablished();
                    }
                } else {
                    cleanup = true;
                    break;
                }
            }
            if (++pktsRcvd == npkts) {  // won't ever break if npkts == 0

                break;
            }
        } else {
            cleanup = true;
            break;
        }
    }

    if (cleanup) {
        close_socket(sockfd);
    }

    return (cleanup ? -1 : pktsRcvd);
}

bool PubControlChannel::mdpConnect() {
    bool cleanup = false;

    if (fdControl == -1) {
        if ((fdControl = make_tcp_socket_client(ipConsumer.c_str(), 0, ipPublisher.c_str(), portPublisher,
                                                connectTimeout)) >= 0) {
            MIDAS_LOG_INFO("consumer connected to producer " << ipPublisher << ":" << portPublisher);
            feedClient->onControlChannelConnected();
            epoll_event inEvent;
            inEvent.events = EPOLLIN;
            inEvent.data.fd = fdControl;
            if (-1 == epoll_ctl(fdEPOLL, EPOLL_CTL_ADD, fdControl, &inEvent)) {
                MIDAS_LOG_ERROR("epoll_ctl failed: " << errno);
                cleanup = true;
            } else {
                // Now do the HANDSHAKE by sending a connect message
                if (sendConnect() != 0) {
                    cleanup = true;
                }
            }
        }
    }

    if (cleanup) {
        close_socket(fdControl);  // Not removing from epoll as this will automatically be done
        fdControl = -1;
    }
    return fdControl >= 0;
}

/**
 * Send a disconnect msg to publisher
 * @return
 */
bool PubControlChannel::mdpDisconnect() {
    sendDisconnect();
    close_socket(fdControl);
    fdControl = -1;
    connected = false;
    MIDAS_LOG_ERROR("Control channel is now closed!");
    return true;
}

bool PubControlChannel::mdpServer() {
    if (fdControl == -1) {
        if ((fdControl = make_tcp_socket_server(ipPublisher.c_str(), portPublisher)) >= 0) {
            MIDAS_LOG_INFO("producer accepting connection on " << ipPublisher << ":" << portPublisher);
            epoll_event inEvent;
            inEvent.events = EPOLLIN;
            inEvent.data.fd = fdControl;
            if (-1 == epoll_ctl(fdEPOLL, EPOLL_CTL_ADD, fdControl, &inEvent)) {
                MIDAS_LOG_ERROR("epoll_ctl failed: " << errno);
                close_socket(fdControl);
                fdControl = -1;
            }
        }
    }
    return fdControl >= 0;
}

void PubControlChannel::signalConnected() {
    MIDAS_LOG_INFO("Handshake with producer completed");
    pthread_mutex_lock(&connectedLock);
    connected = true;
    pthread_mutex_unlock(&connectedLock);
    pthread_cond_signal(&connectedCV);
}

void PubControlChannel::runloop() {
    if (mode == PubControlChannel::consumer) {
        runloop_consumer();
    } else {
        runloop_publisher();
    }
}

void PubControlChannel::runloop_consumer() {
    assert(mode == PubControlChannel::consumer);

    int nfds;

    if ((fdEPOLL = epoll_create(1)) < 0) {
        MIDAS_LOG_ERROR("Failed on epoll_create " << errno << " " << strerror(errno));
        return;
    }

    epoll_event inEvent;
    inEvent.events = EPOLLIN;
    epoll_event outEvent[128];

    if (create_timer(intervalHeartbeat * 1000, &fdHeartbeatTimer)) {
        inEvent.data.fd = fdHeartbeatTimer;
        epoll_ctl(fdEPOLL, EPOLL_CTL_ADD, fdHeartbeatTimer, &inEvent);
    }

    if (create_timer(intervalKeepTicking /* ms */, &fdKeepTickingTimer)) {
        inEvent.data.fd = fdKeepTickingTimer;
        epoll_ctl(fdEPOLL, EPOLL_CTL_ADD, fdKeepTickingTimer, &inEvent);
    }

    if (create_timer(feedClient->callbackInteval(), &fdPeriodicalTimer)) {
        inEvent.data.fd = fdPeriodicalTimer;
        epoll_ctl(fdEPOLL, EPOLL_CTL_ADD, fdPeriodicalTimer, &inEvent);
    }

    while (!stopped) {
        mdpConnect();

        nfds = epoll_wait(fdEPOLL, outEvent, sizeof(outEvent) / sizeof(outEvent[0]), 1000 /*1 sec*/);
        if (nfds == -1) {
            MIDAS_LOG_ERROR("ERROR: epoll_wait " << strerror(errno));
            continue;
        }
        for (int n = 0; n < nfds; ++n) {
            int fdReady = outEvent[n].data.fd;
            if (fdReady == fdControl) {  // Control events from -producer-
                // Message from producer, receive one pkt at a time - there may be more than one msg in a pkt
                int npkts = this->recv(fdControl, 1);
                if (npkts < 0) {  // socket would have been closed in recv and removed from epoll

                    MIDAS_LOG_WARNING("Publisher hung up?");
                    close_socket(fdControl);
                    fdControl = -1;
                    connected = false;
                    feedClient->onControlChannelDisconnected();
                    ::usleep(pauseMillisecsAfterDisconnect * 1000);
                }
            } else if (fdReady == fdHeartbeatTimer) {
                uint64_t dummy;
                read(fdReady, &dummy, sizeof(dummy));
                if (connected) {
                    sendHeartbeat();
                }
            } else if (fdReady == fdKeepTickingTimer) {
                uint64_t dummy;
                read(fdReady, &dummy, sizeof(dummy));
                if (connected) {
                    int rc;
                    mdsymT s;
                    for (uint32_t nreq = 0; nreq < numReqAtATime; ++nreq) {  // Handle subscribe requests

                        if (!reqSubscribe.get(s)) {
                            break;  // no more requests in queue
                        }
                        if ((rc = sendSubscribe(std::get<0>(s), std::get<1>(s))) != 0) {
                            MIDAS_LOG_WARNING("Failed to send subscribe request for "
                                              << std::get<0>(s) << "." << std::get<1>(s) << ", error: " << rc
                                              << ", will retry");
                            reqSubscribe.put(s);
                            break;  // don't try further at this point
                        }
                    }
                    for (uint32_t nreq = 0; nreq < numReqAtATime; ++nreq) {  // Handle unsubscribe requests
                        if (!reqUnsubscribe.get(s)) {
                            break;  // no more requests in queue
                        }
                        if ((rc = sendUnsubscribe(std::get<0>(s), std::get<1>(s))) != 0) {
                            MIDAS_LOG_WARNING("Failed to send unsubscribe request for "
                                              << std::get<0>(s) << "." << std::get<1>(s) << ", error: " << rc
                                              << ", will retry");
                            reqUnsubscribe.put(s);
                            break;  // don't try further at this point
                        }
                    }
                }
            } else if (fdReady == fdPeriodicalTimer) {
                uint64_t dummy;
                read(fdReady, &dummy, sizeof(dummy));
                if (connected) {
                    feedClient->callbackPeriodically();
                }
            }
        }
    }

    destroy_timer(&fdPeriodicalTimer);
    destroy_timer(&fdKeepTickingTimer);
    destroy_timer(&fdHeartbeatTimer);
    mdpDisconnect();
}

void PubControlChannel::runloop_publisher() {
    assert(mode == PubControlChannel::publisher);

    int nfds;
    epoll_event inEvent;
    epoll_event outEvent[128];

    if ((fdEPOLL = epoll_create(1)) < 0) {
        MIDAS_LOG_ERROR("Failed on epoll_create " << errno << " " << strerror(errno));
        return;
    }

    inEvent.events = EPOLLIN;

    if (create_timer(intervalHeartbeat * 1000, &fdHeartbeatTimer)) {
        inEvent.data.fd = fdHeartbeatTimer;
        epoll_ctl(fdEPOLL, EPOLL_CTL_ADD, fdHeartbeatTimer, &inEvent);
    }

    if (create_timer(intervalKeepTicking /* ms */, &fdKeepTickingTimer)) {
        inEvent.data.fd = fdKeepTickingTimer;
        epoll_ctl(fdEPOLL, EPOLL_CTL_ADD, fdKeepTickingTimer, &inEvent);
    }

    while (!stopped) {
        mdpServer();  // Establish server socket

        nfds = epoll_wait(fdEPOLL, outEvent, sizeof(outEvent) / sizeof(outEvent[0]), 1);
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
                    feedServer->register_feed_client(fdClient, ipClient, portClient);
                    if (ipClient) {
                        free(ipClient);
                    }
                    inEvent.data.fd = fdClient;
                    epoll_ctl(fdEPOLL, EPOLL_CTL_ADD, fdClient, &inEvent);
                }
            } else if (fdReady == fdHeartbeatTimer) {
                uint64_t dummy;
                read(fdReady, &dummy, sizeof(dummy));
            } else if (fdReady == fdKeepTickingTimer) {
                uint64_t dummy;
                read(fdReady, &dummy, sizeof(dummy));
                feedServer->on_house_keeping();
            } else {
                // Message from an consumer, receive one pkt at a time - there may be more than one msg in a pkt
                int npkts = this->recv(fdReady, 1);
                if (npkts < 0) {  // Socket would have been closed by recv and removed from epoll

                    MIDAS_LOG_WARNING("Consumer(" << fdReady << ") hung up?");
                    feedServer->unregister_feed_client(fdReady);
                } else {
                    feedServer->update_client_heartbeat(fdReady);
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
    cc->runloop();  // blocks here
    return nullptr;
}

int PubControlChannel::sendConnect() {
    assert(mode == PubControlChannel::consumer);

    MIDAS_LOG_INFO("Session: " << feedClient->session << ", Sequence: " << (controlSeqNo + 1)
                               << ", Client_UserID: " << feedClient->user() << ", Client_Name: " << feedClient->name()
                               << ", Client_PID: " << feedClient->pid() << ", Client_ID: " << feedClient->clientid()
                               << ",  Client_Flags: " << feedClient->subscriptionFlags());

    uint8_t buffer[sizeof(Header) + sizeof(CtrlConnect)];
    Header* hp = (Header*)buffer;
    hp->session = feedClient->session();
    hp->seq = ++controlSeqNo;
    hp->streamId = STREAM_ID_CONTROL_CHANNEL;
    hp->count = 1;
    hp->size = sizeof(CtrlConnect);

    CtrlConnect dummy;
    CtrlConnect* cp = (CtrlConnect*)(buffer + sizeof(Header));
    *cp = dummy;
    cp->clientPid = feedClient->pid();
    cp->clientId = feedClient->clientid();
    cp->flags = feedClient->subscriptionFlags();
    memcpy(cp->user, feedClient->user().c_str(), feedClient->user().length());
    memcpy(cp->pwd, feedClient->pwd().c_str(), feedClient->pwd().length());
    memcpy(cp->shmKey, feedClient->name().c_str(), feedClient->name().length());

    hp->xmitts = midas::ntime();

    return (write_socket(fdControl, buffer, sizeof(Header) + hp->size) == sizeof(buffer) ? CONSUMER_OK
                                                                                         : CONSUMER_NOT_CONNECTED);
}

int PubControlChannel::sendDisconnect() {
    assert(mode == PubControlChannel::consumer);

    MIDAS_LOG_INFO("Session: " << feedClient->session << ", Sequence: " << (controlSeqNo + 1)
                               << ", Client_UserID: " << feedClient->user() << ", Client_Name: " << feedClient->name()
                               << ", Client_PID: " << feedClient->pid() << ", Client_ID: " << feedClient->clientid());

    uint8_t buffer[sizeof(Header) + sizeof(CtrlDisconnect)];

    Header* hp = (Header*)buffer;
    hp->session = feedClient->session();
    hp->seq = ++controlSeqNo;
    hp->streamId = STREAM_ID_CONTROL_CHANNEL;
    hp->count = 1;
    hp->size = sizeof(CtrlDisconnect);

    CtrlDisconnect dummy;
    CtrlDisconnect* dcp = (CtrlDisconnect*)(buffer + sizeof(Header));
    *dcp = dummy;
    dcp->cpid = feedClient->pid();
    hp->xmitts = midas::ntime();
    return (write_socket(fdControl, buffer, sizeof(Header) + hp->size) == sizeof(buffer) ? CONSUMER_OK
                                                                                         : CONSUMER_NOT_CONNECTED);
}

int PubControlChannel::sendSubscribe(std::string const& mdTick, uint16_t mdExch) {
    assert(mode == PubControlChannel::consumer);

    int rc = CONSUMER_OK;

    if (!control_thread || !connected) {
        reqSubscribe.put(std::make_tuple(mdTick, mdExch));
    } else {
        MIDAS_LOG_INFO("Session: " << feedClient->session() << ", Sequence: " << (controlSeqNo + 1) ", mdTick: "
                                   << mdTick << ", mdExch: " << mdExch);

        uint8_t buffer[sizeof(Header) + sizeof(CtrlSubscribe)];

        Header* hp = (Header*)buffer;
        hp->session = feedClient->session();
        hp->seq = ++controlSeqNo;
        hp->streamId = STREAM_ID_CONTROL_CHANNEL;
        hp->count = 1;
        hp->size = sizeof(CtrlSubscribe);

        CtrlSubscribe dummy;
        CtrlSubscribe* sp = (CtrlSubscribe*)(buffer + sizeof(Header));
        *sp = dummy;
        assert(sizeof(sp->symbol) > mdTick.length());
        memcpy(sp->symbol, mdTick.c_str(), mdTick.length());
        sp->exchange = mdExch;
        hp->xmitts = midas::ntime();
        return (write_socket(fdControl, buffer, sizeof(Header) + hp->size) == sizeof(buffer) ? CONSUMER_OK
                                                                                             : CONSUMER_NOT_CONNECTED);
    }

    return rc;
}

int PubControlChannel::sendUnsubscribe(std::string const& mdTick, uint16_t mdExch) {
    assert(mode == PubControlChannel::consumer);

    int rc = CONSUMER_OK;

    if (!control_thread || !connected) {
        reqUnsubscribe.put(std::make_tuple(mdTick, mdExch));
    } else {
        MIDAS_LOG_INFO("Session: " << feedClient->session() << ", Sequence: " << (controlSeqNo + 1) ", mdTick: "
                                   << mdTick << ", mdExch: " << mdExch);

        uint8_t buffer[sizeof(Header) + sizeof(CtrlUnsubscribe)];

        Header* hp = (Header*)buffer;
        hp->session = feedClient->session();
        hp->seq = ++controlSeqNo;
        hp->streamId = STREAM_ID_CONTROL_CHANNEL;
        hp->count = 1;
        hp->size = sizeof(CtrlUnsubscribe);

        CtrlUnsubscribe dummy;
        CtrlUnsubscribe* usp = (CtrlUnsubscribe*)(buffer + sizeof(Header));
        *usp = dummy;
        memcpy(usp->symbol, mdTick.c_str(), mdTick.length());
        usp->exchange = mdExch;
        hp->xmitts = midas::ntime();
        return (write_socket(fdControl, buffer, sizeof(Header) + hp->size) == sizeof(buffer) ? CONSUMER_OK
                                                                                             : CONSUMER_NOT_CONNECTED);
    }

    return rc;
}

int PubControlChannel::sendHeartbeat() {
    uint8_t buffer[sizeof(Header)];

    Header* hp = (Header*)buffer;
    hp->session = feedClient->session();
    hp->seq = ++controlSeqNo;
    hp->streamId = STREAM_ID_CONTROL_CHANNEL;
    hp->count = 0;
    hp->size = 0;
    hp->xmitts = midas::ntime();
    return (write_socket(fdControl, buffer, sizeof(Header) + hp->size) == sizeof(buffer) ? CONSUMER_OK
                                                                                         : CONSUMER_NOT_CONNECTED);
}

bool PubControlChannel::waitUntilConnected() {
    assert(mode == PubControlChannel::consumer);

    struct timespec expiry;
    clock_gettime(CLOCK_REALTIME, &expiry);
    expiry.tv_sec += (connectTimeout + 1);
    int rc = 0;
    pthread_mutex_lock(&connectedLock);
    while (!connected) {
        rc = pthread_cond_timedwait(&connectedCV, &connectedLock, &expiry);
        if (ETIMEDOUT == rc) break;
    }
    if (rc == ETIMEDOUT) {
        MIDAS_LOG_ERROR("consumer timed out while connecting to producer");
    } else if (connected) {
        MIDAS_LOG_INFO("consumer connected to producer");
    }
    pthread_mutex_unlock(&connectedLock);
    return connected;
}
