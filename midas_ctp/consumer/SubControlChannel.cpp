#include <midas/MidasConfig.h>
#include <midas/md/MdDefs.h>
#include <net/raw/MdProtocol.h>
#include <net/raw/RawSocket.h>
#include <utils/MidasUtils.h>
#include "Consumer.h"
#include "SubControlChannel.h"

using namespace std;
using namespace midas;

SubControlChannel::SubControlChannel(std::string const& key, Consumer* consumer_) : consumer(consumer_) {
    pthread_mutex_init(&connectedLock, nullptr);
    pthread_cond_init(&connectedCV, nullptr);
    ipConsumer = midas::get_cfg_value<string>(key, "consumer_ip");
    ipPublisher = midas::get_cfg_value<string>(key, "publisher_ip");
    portPublisher = midas::get_cfg_value<uint16_t>(key, "publisher_port");
    intervalHeartbeat = midas::get_cfg_value<uint32_t>(key, "heartbeat_interval", 1);
    MIDAS_LOG_INFO("producer : " << ipPublisher << ":" << portPublisher
                                 << " heartbeat_interval: " << intervalHeartbeat);
}

SubControlChannel::~SubControlChannel() {
    MIDAS_LOG_INFO("Destroying control channel");
    pthread_cond_destroy(&connectedCV);
    pthread_mutex_destroy(&connectedLock);
}

/**
 * reading 'npkts' message packets where one message packet => [1xHeader + nxMsgs]
 * @param sockfd
 * @param npkts continue reading if 'npkts' is 0
 * @return
 */
int SubControlChannel::recv(int sockfd, int npkts) {
    uint8_t buffer[1024];
    int pktsRcvd = 0;
    bool cleanup = false;

    for (;;) {
        Header hdr;
        ssize_t n = read_socket(sockfd, (uint8_t*)&hdr, sizeof(Header));  // reader Header first
        if (n > 0) {
            if (hdr.size == 0) {
                // heartbeat
            } else {
                n = read_socket(sockfd, buffer, hdr.size);
                if (n > 0) {
                    bool isConnected = false;
                    uint8_t* pData = buffer;
                    for (auto c = 0; c < hdr.count; ++c) {
                        switch (*pData) {
                            case CTRL_CONNECT_RESPONSE_TYPE:
                                MIDAS_LOG_INFO("Dispatching on_connect_response");
                                consumer->on_connect_response(&hdr, (CtrlConnectResponse*)pData, &isConnected);
                                break;
                            case CTRL_DISCONNECT_RESPONSE_TYPE:
                                MIDAS_LOG_INFO("Dispatching on_disconnect_response");
                                consumer->on_disconnect_response(&hdr, (CtrlDisconnectResponse*)pData);
                                break;
                            case CTRL_SUBSCRIBE_RESPONSE_TYPE:
                                consumer->on_subscribe_response(&hdr, (CtrlSubscribeResponse*)pData);
                                break;
                            case CTRL_UNSUBSCRIBE_RESPONSE_TYPE:
                                consumer->on_unsubscribe_response(&hdr, (CtrlUnsubscribeResponse*)pData);
                                break;
                            default:
                                break;
                        }
                        pData += 1;             // advance by 1 byte to size field
                        pData += (*pData - 1);  // advance to next message
                    }
                    // Multiple CtrlConnectResponse is delivered in message unit
                    // Call 'signal_connected' only when the entire unit is processed
                    if (isConnected) {
                        signal_connected();
                        consumer->on_control_channel_established();
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

    if (cleanup) close_socket(sockfd);
    return (cleanup ? -1 : pktsRcvd);
}

bool SubControlChannel::connect() {
    bool cleanup = false;
    if (fdControl == -1) {
        if ((fdControl = make_tcp_socket_client(ipConsumer.c_str(), 0, ipPublisher.c_str(), portPublisher,
                                                connectTimeout)) >= 0) {
            MIDAS_LOG_INFO("consumer connected to producer " << ipPublisher << ":" << portPublisher);
            consumer->on_control_channel_connected();
            epoll_event inEvent;
            inEvent.events = EPOLLIN;
            inEvent.data.fd = fdControl;
            if (-1 == epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fdControl, &inEvent)) {
                MIDAS_LOG_ERROR("epoll_ctl failed: " << errno);
                cleanup = true;
            } else {
                if (send_connect() != 0) {  // Now do the HANDSHAKE
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
bool SubControlChannel::disconnect() {
    send_disconnect();
    close_socket(fdControl);
    fdControl = -1;
    connected = false;
    MIDAS_LOG_ERROR("Control channel is now closed!");
    return true;
}

void SubControlChannel::signal_connected() {
    MIDAS_LOG_INFO("Handshake with producer completed");
    pthread_mutex_lock(&connectedLock);
    connected = true;
    pthread_mutex_unlock(&connectedLock);
    pthread_cond_signal(&connectedCV);
}

void SubControlChannel::run_loop() {
    int nfds;
    if ((fdEpoll = epoll_create(1)) < 0) {
        MIDAS_LOG_ERROR("Failed on epoll_create " << errno << " " << strerror(errno));
        return;
    }

    epoll_event inEvent;
    inEvent.events = EPOLLIN;
    epoll_event outEvent[128];

    if (create_timer(intervalHeartbeat * 1000, &fdHeartbeatTimer)) {
        inEvent.data.fd = fdHeartbeatTimer;
        epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fdHeartbeatTimer, &inEvent);
    }

    if (create_timer(consumer->callback_interval(), &fdPeriodicalTimer)) {
        inEvent.data.fd = fdPeriodicalTimer;
        epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fdPeriodicalTimer, &inEvent);
    }

    while (!stopped) {
        connect();
        nfds = epoll_wait(fdEpoll, outEvent, sizeof(outEvent) / sizeof(outEvent[0]), 1000 /*1 sec*/);
        if (nfds == -1) {
            if (errno != EINTR) MIDAS_LOG_ERROR("epoll_wait " << strerror(errno));
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
                    consumer->on_control_channel_disconnected();
                    ::usleep(pauseMillisecsAfterDisconnect * 1000);
                }
            } else if (fdReady == fdHeartbeatTimer) {
                uint64_t dummy;
                read(fdReady, &dummy, sizeof(dummy));
                if (connected) {
                    send_heartbeat();
                }
            } else if (fdReady == fdKeepTickingTimer) {
                uint64_t dummy;
                read(fdReady, &dummy, sizeof(dummy));
                if (connected) {
                    int rc;
                    mdsymT s;
                    for (uint32_t nreq = 0; nreq < numReqAtATime; ++nreq) {  // Handle subscribe requests
                        if (!reqSubscribe.get(s)) break;                     // no more requests in queue

                        if ((rc = send_subscribe(std::get<0>(s), std::get<1>(s))) != 0) {
                            MIDAS_LOG_WARNING("Failed to submit subscribe request for "
                                              << std::get<0>(s) << "." << std::get<1>(s) << ", error: " << rc
                                              << ", will retry");
                            reqSubscribe.put(s);
                            break;  // don't try further at this point
                        }
                    }
                    for (uint32_t nreq = 0; nreq < numReqAtATime; ++nreq) {  // Handle unsubscribe requests
                        if (!reqUnsubscribe.get(s)) break;                   // no more requests in queue

                        if ((rc = send_unsubscribe(std::get<0>(s), std::get<1>(s))) != 0) {
                            MIDAS_LOG_WARNING("Failed to submit unsubscribe request for "
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
                    consumer->callback_periodically();
                }
            }
        }
    }

    destroy_timer(&fdPeriodicalTimer);
    destroy_timer(&fdKeepTickingTimer);
    destroy_timer(&fdHeartbeatTimer);
    disconnect();
}

void* SubControlChannel::run(void* arg) {
    SubControlChannel* cc = static_cast<SubControlChannel*>(arg);
    MIDAS_LOG_INFO("Starting control channel thread, id: " << midas::get_tid());
    cc->run_loop();  // blocks here
    return nullptr;
}

int SubControlChannel::send_connect() {
    MIDAS_LOG_INFO("Session: " << consumer->session << ", Sequence: " << (controlSeqNo + 1)
                               << ", Client_Name: " << consumer->shmName << ", Client_PID: " << consumer->pid
                               << ", Client_ID: " << consumer->clientId
                               << ",  Client_Flags: " << consumer->subscriptionFlags);

    uint8_t buffer[sizeof(Header) + sizeof(CtrlConnect)];
    Header* pHeader = (Header*)buffer;
    pHeader->session = consumer->session;
    pHeader->seq = ++controlSeqNo;
    pHeader->streamId = STREAM_ID_CONTROL_CHANNEL;
    pHeader->count = 1;
    pHeader->size = sizeof(CtrlConnect);

    CtrlConnect dummy;
    CtrlConnect* cc = (CtrlConnect*)(buffer + sizeof(Header));
    *cc = dummy;
    cc->clientPid = consumer->pid;
    cc->clientId = static_cast<uint8_t>(consumer->clientId);
    cc->flags = consumer->subscriptionFlags;
    memcpy(cc->user, consumer->userName.c_str(), consumer->userName.length());
    memcpy(cc->pwd, consumer->password.c_str(), consumer->password.length());
    memcpy(cc->shmKey, consumer->shmName.c_str(), consumer->shmName.length());

    pHeader->transmitTimestamp = midas::ntime();
    return (write_socket(fdControl, buffer, sizeof(Header) + pHeader->size) == sizeof(buffer) ? CONSUMER_OK
                                                                                              : CONSUMER_NOT_CONNECTED);
}

int SubControlChannel::send_disconnect() {
    MIDAS_LOG_INFO("Session: " << consumer->session << ", Sequence: " << (controlSeqNo + 1)
                               << ", Client_UserID: " << consumer->userName << ", Client_Name: " << consumer->shmName
                               << ", Client_PID: " << consumer->pid << ", Client_ID: " << consumer->clientId);

    uint8_t buffer[sizeof(Header) + sizeof(CtrlDisconnect)];

    Header* pHader = (Header*)buffer;
    pHader->session = consumer->session;
    pHader->seq = ++controlSeqNo;
    pHader->streamId = STREAM_ID_CONTROL_CHANNEL;
    pHader->count = 1;
    pHader->size = sizeof(CtrlDisconnect);

    CtrlDisconnect dummy;
    CtrlDisconnect* cd = (CtrlDisconnect*)(buffer + sizeof(Header));
    *cd = dummy;
    cd->cpid = consumer->pid;
    pHader->transmitTimestamp = midas::ntime();
    return (write_socket(fdControl, buffer, sizeof(Header) + pHader->size) == sizeof(buffer) ? CONSUMER_OK
                                                                                             : CONSUMER_NOT_CONNECTED);
}

uint8_t SubControlChannel::send_subscribe(std::string const& mdTick, uint16_t mdExch) {
    uint8_t rc = CONSUMER_OK;

    if (!connected) {
        reqSubscribe.put(std::make_tuple(mdTick, mdExch));
    } else {
        MIDAS_LOG_INFO("Session: " << consumer->session << ", Sequence: " << (controlSeqNo + 1)
                                   << ", md_tick: " << mdTick << ", exchange: " << mdExch);

        uint8_t buffer[sizeof(Header) + sizeof(CtrlSubscribe)];

        Header* pHeader = (Header*)buffer;
        pHeader->session = consumer->session;
        pHeader->seq = ++controlSeqNo;
        pHeader->streamId = STREAM_ID_CONTROL_CHANNEL;
        pHeader->count = 1;
        pHeader->size = sizeof(CtrlSubscribe);

        CtrlSubscribe dummy;
        CtrlSubscribe* cs = (CtrlSubscribe*)(buffer + sizeof(Header));
        *cs = dummy;
        memcpy(cs->symbol, mdTick.c_str(), mdTick.length());
        cs->exchange = mdExch;
        pHeader->transmitTimestamp = midas::ntime();
        return (write_socket(fdControl, buffer, sizeof(Header) + pHeader->size) == sizeof(buffer)
                    ? CONSUMER_OK
                    : CONSUMER_NOT_CONNECTED);
    }
    return rc;
}

uint8_t SubControlChannel::send_unsubscribe(std::string const& mdTick, uint16_t mdExch) {
    uint8_t rc = CONSUMER_OK;

    if (!connected) {
        reqUnsubscribe.put(std::make_tuple(mdTick, mdExch));
    } else {
        MIDAS_LOG_INFO("send unsubscribe Session: " << consumer->session << ", Sequence: " << (controlSeqNo + 1)
                                                    << ", md_tick: " << mdTick << ", exchange: " << mdExch);

        uint8_t buffer[sizeof(Header) + sizeof(CtrlUnsubscribe)];

        Header* pHeader = (Header*)buffer;
        pHeader->session = consumer->session;
        pHeader->seq = ++controlSeqNo;
        pHeader->streamId = STREAM_ID_CONTROL_CHANNEL;
        pHeader->count = 1;
        pHeader->size = sizeof(CtrlUnsubscribe);

        CtrlUnsubscribe dummy;
        CtrlUnsubscribe* cu = (CtrlUnsubscribe*)(buffer + sizeof(Header));
        *cu = dummy;
        memcpy(cu->symbol, mdTick.c_str(), mdTick.length());
        cu->exchange = mdExch;
        pHeader->transmitTimestamp = midas::ntime();
        return (write_socket(fdControl, buffer, sizeof(Header) + pHeader->size) == sizeof(buffer)
                    ? CONSUMER_OK
                    : CONSUMER_NOT_CONNECTED);
    }
    return rc;
}

int SubControlChannel::send_heartbeat() {
    uint8_t buffer[sizeof(Header)];

    Header* pHeader = (Header*)buffer;
    pHeader->session = consumer->session;
    pHeader->seq = ++controlSeqNo;
    pHeader->streamId = STREAM_ID_CONTROL_CHANNEL;
    pHeader->count = 0;
    pHeader->size = 0;
    pHeader->transmitTimestamp = midas::ntime();
    return (write_socket(fdControl, buffer, sizeof(Header) + pHeader->size) == sizeof(buffer) ? CONSUMER_OK
                                                                                              : CONSUMER_NOT_CONNECTED);
}

bool SubControlChannel::wait_until_connected() {
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
