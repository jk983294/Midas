#include <fcntl.h>
#include <net/raw/RawSocket.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include "ConsumerProxy.h"
#include "DataChannel.h"
#include "MdTicker.h"
#include "Publisher.h"

using namespace std;
using namespace midas;

ConsumerProxy::ConsumerProxy(int sockFdConsumer_, std::string const& ipConsumer_, uint16_t portConsumer_,
                             Publisher const* publisher_, DataChannel* dataChannel_, uint8_t numDataQueues_)
    : publisher(publisher_),
      sockFdConsumer(sockFdConsumer_),
      ipConsumer(ipConsumer_),
      portConsumer(portConsumer_),
      numDataQueues(numDataQueues_),
      dataChannel(dataChannel_) {
    MIDAS_LOG_INFO("Creating ConsumerProxy for consumer " << ipConsumer << ":" << portConsumer
                                                          << ", Sock: " << sockFdConsumer);
}

ConsumerProxy::~ConsumerProxy() {
    MIDAS_LOG_INFO("Destroying ConsumerProxy for consumer " << ipConsumer << ":" << portConsumer);
    close_socket(sockFdConsumer);
}

void ConsumerProxy::set_details(CtrlConnect const& ctrl) {
    clientId = ctrl.clientId;
    pidConsumer = ctrl.clientPid;
    userConsumer = ctrl.user;
    shmKeyConsumer = ctrl.shmKey;
    subscriptionFlags = ctrl.flags;
}

/*
 * PubControlChannel->Publisher->ConsumerProxy::on_connect
 */
void ConsumerProxy::on_connect(uint8_t status) {
    MIDAS_LOG_INFO("Client_IP: " << ipConsumer << ", Client_Port: " << portConsumer << ", Client_PID: " << pidConsumer
                                 << ", Client_UserID: " << userConsumer << ", Client_ID: " << clientId
                                 << ", Client_Key: " << shmKeyConsumer << ", Client_Sock: " << sockFdConsumer
                                 << ", Client_Flags: " << subscriptionFlags);

    if (status == CONNECT_STATUS_OK) {
        for (decltype(numDataQueues) u = 0; u < numDataQueues; ++u) {
            std::ostringstream oss;
            oss << publisher->name << "." << shmKeyConsumer << "." << static_cast<int>(u);
            status = dataChannel->create_queue(oss.str());
            if (status != CONNECT_STATUS_OK) break;
        }
    }
    send_connect_response(status);
}

/*
 * ConsumerProxy::send_connect_response-><<<tcp-socket>>>->PubControlChannel-on-Consumer
 */
void ConsumerProxy::send_connect_response(uint8_t status) {
    CtrlConnectResponse dummy;
    uint8_t buffer[1024];
    Header* hp = (Header*)buffer;
    hp->session = publisher->session;
    hp->seq = ++controlSeqNo;
    hp->streamId = STREAM_ID_CONTROL_CHANNEL;

    uint8_t mcnt = 0;
    uint8_t* pld = buffer + sizeof(Header);
    uint16_t pldsz = 0;

    if (status == CONNECT_STATUS_OK) {
        // add response to communicate shared memory queue names, One CtrlConnectResponse for each queue
        for (auto&& name : dataChannel->queueNames) {
            CtrlConnectResponse* crsp = (CtrlConnectResponse*)(pld + pldsz);
            *crsp = dummy;
            crsp->cid = clientId;
            crsp->status = CONNECT_STATUS_OK;
            crsp->shmType = SHM_TYPE_EVENT_QUEUE;
            snprintf(crsp->shmPath, sizeof crsp->shmPath, "%s", name.c_str());  // protect buffer overrun
            pldsz = static_cast<uint16_t>(pldsz + sizeof(CtrlConnectResponse));
            mcnt = static_cast<uint8_t>(mcnt + 1);
        }

        // add one response to communicate shared memory book cache name
        for (auto&& mapRec : publisher->bookCaches) {
            CtrlConnectResponse* crsp = (CtrlConnectResponse*)(pld + pldsz);
            *crsp = dummy;
            crsp->cid = clientId;
            crsp->status = CONNECT_STATUS_OK;
            crsp->shmType = SHM_TYPE_BOOK_CACHE;
            snprintf(crsp->shmPath, sizeof crsp->shmPath, "%s",
                     mapRec.second->name().c_str());  // protect buffer overrun
            crsp->shmSize = static_cast<uint32_t>(mapRec.second->size());
            crsp->exchange = mapRec.first;
            pldsz = static_cast<uint16_t>(pldsz + sizeof(CtrlConnectResponse));
            mcnt = static_cast<uint8_t>(mcnt + 1);
        }
    } else {
        CtrlConnectResponse* crsp = (CtrlConnectResponse*)(pld + pldsz);
        *crsp = dummy;
        crsp->cid = clientId;
        crsp->status = status;
        pldsz = static_cast<uint16_t>(pldsz + sizeof(CtrlConnectResponse));
        mcnt = static_cast<uint8_t>(mcnt + 1);
    }

    hp->count = mcnt;
    hp->size = pldsz;
    hp->xmitts = midas::ntime();
    write_socket(sockFdConsumer, buffer, sizeof(Header) + hp->size);
}

/*
 * PubControlChannel->Publisher->ConsumerProxy::on_disconnect
 */
void ConsumerProxy::on_disconnect() { send_disconnect_response(); }

/*
 * ConsumerProxy::send_disconnect_response-><<<tcp-socket>>>->PubControlChannel-on-Consumer
 */
void ConsumerProxy::send_disconnect_response() {
    uint8_t buffer[sizeof(Header) + sizeof(CtrlDisconnectResponse)];

    Header* hp = (Header*)buffer;
    hp->session = publisher->session;
    hp->seq = ++controlSeqNo;
    hp->streamId = STREAM_ID_CONTROL_CHANNEL;
    hp->count = 1;
    hp->size = sizeof(CtrlDisconnectResponse);

    CtrlDisconnectResponse dummy;
    CtrlDisconnectResponse* dcrsp = (CtrlDisconnectResponse*)(buffer + sizeof(Header));
    *dcrsp = dummy;
    dcrsp->status = CONNECT_STATUS_OK;
    hp->xmitts = midas::ntime();
    write_socket(sockFdConsumer, buffer, sizeof(Header) + hp->size);
}

/*
 * ConsumerProxy::send_subscribe_response-><<<tcp-socket>>>->PubControlChannel-on-Consumer
 */
void ConsumerProxy::send_subscribe_response(MdTicker const& ticker) {
    uint8_t status;
    uint16_t locate;

    if (MdTicker::State::badsymbol == ticker.state) {
        status = SUBSCRIBE_STATUS_BAD_SYMBOL;
        locate = UINT16_MAX;
    } else {
        status = CONNECT_STATUS_OK;
        locate = ticker.locate;
    }

    send_subscribe_response(status, locate, ticker.mdTick.c_str(), ticker.exchange);
}

void ConsumerProxy::send_subscribe_response(uint8_t status, uint16_t locate, char const* mdTick, uint16_t mdExch) {
    uint8_t buffer[sizeof(Header) + sizeof(CtrlSubscribeResponse)];

    Header* hp = (Header*)buffer;
    hp->session = publisher->session;
    hp->seq = ++controlSeqNo;
    hp->streamId = STREAM_ID_CONTROL_CHANNEL;
    hp->count = 1;
    hp->size = sizeof(CtrlSubscribeResponse);

    CtrlSubscribeResponse dummy;
    CtrlSubscribeResponse* srsp = (CtrlSubscribeResponse*)(buffer + sizeof(Header));
    *srsp = dummy;
    srsp->status = status;
    srsp->locate = locate;
    memcpy(srsp->symbol, mdTick, strlen(mdTick));
    srsp->exchange = mdExch;
    hp->xmitts = midas::ntime();
    write_socket(sockFdConsumer, buffer, sizeof(Header) + hp->size);
}

/*
 * DataSource->ConsumerProxy::send_unsubscribe_response-><<<tcp-socket>>>->PubControlChannel-on-Consumer
 */
void ConsumerProxy::send_unsubscribe_response(MdTicker const& ticker) {
    uint8_t buffer[sizeof(Header) + sizeof(CtrlUnsubscribeResponse)];

    Header* hp = (Header*)buffer;
    hp->session = publisher->session;
    hp->seq = ++controlSeqNo;
    hp->streamId = STREAM_ID_CONTROL_CHANNEL;
    hp->count = 1;
    hp->size = sizeof(CtrlUnsubscribeResponse);

    CtrlUnsubscribeResponse dummy;
    CtrlUnsubscribeResponse* usrsp = (CtrlUnsubscribeResponse*)(buffer + sizeof(Header));
    *usrsp = dummy;
    usrsp->status = CONNECT_STATUS_OK;
    usrsp->locate = ticker.locate;
    memcpy(usrsp->symbol, ticker.mdTick.c_str(), ticker.mdTick.length());
    usrsp->exchange = ticker.exchange;
    hp->xmitts = midas::ntime();

    write_socket(sockFdConsumer, buffer, sizeof(Header) + hp->size);
}

/*
 * DataSource->MdTicker->ConsumerProxy::send_book_refreshed-><<<Shared Memory Queue>>>->DataChannel-on-Consumer
 */
void ConsumerProxy::send_book_refreshed(MdTicker const& ticker) {
    if (!(subscriptionFlags & FLAG_SEND_BOOK_REFRESHED)) return;

    uint8_t mdThreadNum = ticker.dataSource.queueIndex;
    uint64_t mdSequence = ticker.dataSource.step_sequence();

    // write directly into shared memory
    uint8_t* datap = dataChannel->write_prepare(mdThreadNum, sizeof(Header) + sizeof(DataBookRefreshed));
    if (datap) {
        Header* hp = (Header*)datap;
        hp->session = publisher->session;
        hp->seq = mdSequence;
        hp->streamId = mdThreadNum;
        hp->count = 1;
        hp->size = sizeof(DataBookRefreshed);

        DataBookRefreshed dummy;
        DataBookRefreshed* brp = (DataBookRefreshed*)(datap + sizeof(Header));
        *brp = dummy;
        brp->locate = ticker.locate;
        memcpy(brp->symbol, ticker.mdTick.c_str(), ticker.mdTick.length());
        brp->exchange = ticker.exchange;
        hp->xmitts = midas::ntime();
        dataChannel->write_commit(mdThreadNum, sizeof(Header) + sizeof(DataBookRefreshed));
    }
}

/*
 * DataSource->MdTicker->ConsumerProxy::send_book_changed-><<<Shared Memory Queue>>>->DataChannel-on-Consumer
 */
void ConsumerProxy::send_book_changed(MdTicker const& ticker) {
    if (!(subscriptionFlags & FLAG_SEND_BOOK_CHANGED) || !publisher->sendBookUpdate) return;

    uint8_t mdThreadNum = ticker.dataSource.queueIndex;
    uint64_t mdSequence = ticker.dataSource.step_sequence();

    uint8_t* datap = dataChannel->write_prepare(
        mdThreadNum, sizeof(Header) + sizeof(DataBookChanged));  // write directly into shared memory
    if (datap) {
        Header* hp = (Header*)datap;
        hp->session = publisher->session;
        hp->seq = mdSequence;
        hp->streamId = mdThreadNum;
        hp->count = 1;
        hp->size = sizeof(DataBookChanged);

        DataBookChanged dummy;
        DataBookChanged* bcp = (DataBookChanged*)(datap + sizeof(Header));
        *bcp = dummy;
        bcp->locate = ticker.locate;
        memcpy(bcp->symbol, ticker.mdTick.c_str(), ticker.mdTick.length());
        bcp->exchange = ticker.exchange;
        bcp->bookchanged.side = ticker.bookChangedSide;
        bcp->bookchanged.timestamps.srcReceive = ticker.srcReceiveTime;
        bcp->bookchanged.timestamps.srcTransmit = ticker.srcTransmitTime;
        bcp->bookchanged.timestamps.producerReceive = ticker.producerReceiveTime;
        bcp->bookchanged.timestamps.producerTransmit = hp->xmitts = midas::ntime();
        dataChannel->write_commit(mdThreadNum, sizeof(Header) + sizeof(DataBookChanged));
    }
}

/*
 * DataSource->MdTicker->ConsumerProxy::send_trading_action-><<<Shared Memory Queue>>>->DataChannel-on-Consumer
 */
void ConsumerProxy::send_trading_action(MdTicker const& ticker, uint16_t exch) {
    if (!(subscriptionFlags & FLAG_SEND_TRADING_ACTION)) return;

    uint8_t mdThreadNum = ticker.dataSource.queueIndex;
    uint64_t mdSequence = ticker.dataSource.step_sequence();

    uint8_t* datap = dataChannel->write_prepare(
        mdThreadNum, sizeof(Header) + sizeof(DataTradingAction));  // write directly into shared memory
    if (datap) {
        Header* hp = (Header*)datap;
        hp->session = publisher->session;
        hp->seq = mdSequence;
        hp->streamId = mdThreadNum;
        hp->count = 1;
        hp->size = sizeof(DataTradingAction);

        DataTradingAction dummy;
        DataTradingAction* tap = (DataTradingAction*)(datap + sizeof(Header));
        *tap = dummy;
        tap->locate = ticker.locate;
        memcpy(tap->symbol, ticker.mdTick.c_str(), ticker.mdTick.length());
        tap->exchange = exch;
        tap->tradingAction = ticker.tradingAction;
        tap->tradingAction.timestamps.srcReceive = ticker.srcReceiveTime;
        tap->tradingAction.timestamps.srcTransmit = ticker.srcTransmitTime;
        tap->tradingAction.timestamps.producerReceive = ticker.producerReceiveTime;
        tap->tradingAction.timestamps.producerTransmit = hp->xmitts = midas::ntime();
        dataChannel->write_commit(mdThreadNum, sizeof(Header) + sizeof(DataTradingAction));
    }
}

/*
 * DataSource->ConsumerProxy::send_data_heartbeat-><<<Shared Memory Queue>>>->DataChannel-on-Consumer
 */
void ConsumerProxy::send_data_heartbeat(uint8_t queueIndex, uint64_t stepSequence) {
    if (!(subscriptionFlags & FLAG_SEND_DATA_HEARTBEAT)) return;

    uint8_t* datap = dataChannel->write_prepare(queueIndex, sizeof(Header));  // write directly into shared memory
    if (datap) {
        Header* hp = (Header*)datap;
        hp->session = publisher->session;
        hp->seq = stepSequence;
        hp->streamId = queueIndex;
        hp->count = 0;
        hp->size = 0;  // size 0 okay here -- heartbeat indicator
        hp->xmitts = midas::ntime();
        dataChannel->write_commit(queueIndex, sizeof(Header));
    }
}

void ConsumerProxy::count_subscribe(std::string const& symbol, uint16_t exch) {
    if (requestedSubs.count(symbol) == 0) {
        requestedSubs.insert(symbol);
    }
}

void ConsumerProxy::count_unsubscribe(std::string const& symbol, uint16_t exch) {
    if (requestedSubs.count(symbol) > 0) {
        requestedSubs.erase(requestedSubs.find(symbol));
    }
}
