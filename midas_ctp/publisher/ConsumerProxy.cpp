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
    MIDAS_LOG_INFO("Creating ConsumerProxy for consumer " << ipConsumer << ":" << portConsumer << ", Sock: "
                                                          << sockFdConsumer << ", queues: " << numDataQueues);
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
    Header* pHeader = (Header*)buffer;
    pHeader->session = publisher->session;
    pHeader->seq = ++controlSeqNo;
    pHeader->streamId = STREAM_ID_CONTROL_CHANNEL;

    uint8_t msgCount = 0;
    uint8_t* payload = buffer + sizeof(Header);
    uint16_t payloadSize = 0;

    if (status == CONNECT_STATUS_OK) {
        // add response to communicate shared memory queue names, One CtrlConnectResponse for each queue
        for (auto&& name : dataChannel->queueNames) {
            CtrlConnectResponse* ccr = (CtrlConnectResponse*)(payload + payloadSize);
            *ccr = dummy;
            ccr->cid = static_cast<uint8_t>(clientId);
            ccr->status = CONNECT_STATUS_OK;
            ccr->shmType = SHM_TYPE_EVENT_QUEUE;
            snprintf(ccr->shmPath, sizeof ccr->shmPath, "%s", name.c_str());  // protect buffer overrun
            payloadSize = static_cast<uint16_t>(payloadSize + sizeof(CtrlConnectResponse));
            ++msgCount;
        }

        // add one response to communicate shared memory book cache shmName
        for (auto&& mapRec : publisher->bookCaches) {
            CtrlConnectResponse* ccr = (CtrlConnectResponse*)(payload + payloadSize);
            *ccr = dummy;
            ccr->cid = static_cast<uint8_t>(clientId);
            ccr->status = CONNECT_STATUS_OK;
            ccr->shmType = SHM_TYPE_BOOK_CACHE;
            snprintf(ccr->shmPath, sizeof ccr->shmPath, "%s", mapRec.second->name().c_str());  // protect buffer overrun
            ccr->shmSize = static_cast<uint32_t>(mapRec.second->size());
            ccr->exchange = mapRec.first;
            payloadSize = static_cast<uint16_t>(payloadSize + sizeof(CtrlConnectResponse));
            ++msgCount;
        }
    } else {
        CtrlConnectResponse* ccr = (CtrlConnectResponse*)(payload + payloadSize);
        *ccr = dummy;
        ccr->cid = static_cast<uint8_t>(clientId);
        ccr->status = status;
        payloadSize = static_cast<uint16_t>(payloadSize + sizeof(CtrlConnectResponse));
        ++msgCount;
    }

    pHeader->count = msgCount;
    pHeader->size = payloadSize;
    pHeader->transmitTimestamp = midas::ntime();
    write_socket(sockFdConsumer, buffer, sizeof(Header) + pHeader->size);
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

    Header* pHeader = (Header*)buffer;
    pHeader->session = publisher->session;
    pHeader->seq = ++controlSeqNo;
    pHeader->streamId = STREAM_ID_CONTROL_CHANNEL;
    pHeader->count = 1;
    pHeader->size = sizeof(CtrlDisconnectResponse);

    CtrlDisconnectResponse dummy;
    CtrlDisconnectResponse* dcrsp = (CtrlDisconnectResponse*)(buffer + sizeof(Header));
    *dcrsp = dummy;
    dcrsp->status = CONNECT_STATUS_OK;
    pHeader->transmitTimestamp = midas::ntime();
    write_socket(sockFdConsumer, buffer, sizeof(Header) + pHeader->size);
}

/*
 * ConsumerProxy::send_subscribe_response-><<<tcp-socket>>>->PubControlChannel-on-Consumer
 */
void ConsumerProxy::send_subscribe_response(MdTicker const& ticker) {
    uint8_t status;
    uint16_t locate;

    if (MdTicker::State::badSymbol == ticker.state) {
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

    Header* pHeader = (Header*)buffer;
    pHeader->session = publisher->session;
    pHeader->seq = ++controlSeqNo;
    pHeader->streamId = STREAM_ID_CONTROL_CHANNEL;
    pHeader->count = 1;
    pHeader->size = sizeof(CtrlSubscribeResponse);

    CtrlSubscribeResponse dummy;
    CtrlSubscribeResponse* response = (CtrlSubscribeResponse*)(buffer + sizeof(Header));
    *response = dummy;
    response->status = status;
    response->locate = locate;
    memcpy(response->symbol, mdTick, strlen(mdTick));
    response->exchange = mdExch;
    pHeader->transmitTimestamp = midas::ntime();
    write_socket(sockFdConsumer, buffer, sizeof(Header) + pHeader->size);
}

/*
 * DataSource->ConsumerProxy::send_unsubscribe_response-><<<tcp-socket>>>->PubControlChannel-on-Consumer
 */
void ConsumerProxy::send_unsubscribe_response(MdTicker const& ticker) {
    uint8_t buffer[sizeof(Header) + sizeof(CtrlUnsubscribeResponse)];

    Header* pHeader = (Header*)buffer;
    pHeader->session = publisher->session;
    pHeader->seq = ++controlSeqNo;
    pHeader->streamId = STREAM_ID_CONTROL_CHANNEL;
    pHeader->count = 1;
    pHeader->size = sizeof(CtrlUnsubscribeResponse);

    CtrlUnsubscribeResponse dummy;
    CtrlUnsubscribeResponse* response = (CtrlUnsubscribeResponse*)(buffer + sizeof(Header));
    *response = dummy;
    response->status = CONNECT_STATUS_OK;
    response->locate = ticker.locate;
    memcpy(response->symbol, ticker.mdTick.c_str(), ticker.mdTick.length());
    response->exchange = ticker.exchange;
    pHeader->transmitTimestamp = midas::ntime();

    write_socket(sockFdConsumer, buffer, sizeof(Header) + pHeader->size);
}

/*
 * DataSource->MdTicker->ConsumerProxy::send_book_refreshed-><<<Shared Memory Queue>>>->DataChannel-on-Consumer
 */
void ConsumerProxy::send_book_refreshed(MdTicker const& ticker) {
    if (!(subscriptionFlags & FLAG_SEND_BOOK_REFRESHED)) return;

    uint8_t mdThreadNum = ticker.dataSource.queueIndex;
    uint64_t mdSequence = ticker.dataSource.step_sequence();

    // write directly into shared memory
    uint8_t* pData = dataChannel->write_prepare(mdThreadNum, sizeof(Header) + sizeof(DataBookRefreshed));
    if (pData) {
        Header* pHeader = (Header*)pData;
        pHeader->session = publisher->session;
        pHeader->seq = mdSequence;
        pHeader->streamId = mdThreadNum;
        pHeader->count = 1;
        pHeader->size = sizeof(DataBookRefreshed);

        DataBookRefreshed dummy;
        DataBookRefreshed* dbr = (DataBookRefreshed*)(pData + sizeof(Header));
        *dbr = dummy;
        dbr->locate = ticker.locate;
        memcpy(dbr->symbol, ticker.mdTick.c_str(), ticker.mdTick.length());
        dbr->exchange = ticker.exchange;
        pHeader->transmitTimestamp = midas::ntime();
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

    uint8_t* pData = dataChannel->write_prepare(
        mdThreadNum, sizeof(Header) + sizeof(DataBookChanged));  // write directly into shared memory
    if (pData) {
        Header* pHeader = (Header*)pData;
        pHeader->session = publisher->session;
        pHeader->seq = mdSequence;
        pHeader->streamId = mdThreadNum;
        pHeader->count = 1;
        pHeader->size = sizeof(DataBookChanged);

        DataBookChanged dummy;
        DataBookChanged* dbc = (DataBookChanged*)(pData + sizeof(Header));
        *dbc = dummy;
        dbc->locate = ticker.locate;
        memcpy(dbc->symbol, ticker.mdTick.c_str(), ticker.mdTick.length());
        dbc->exchange = ticker.exchange;
        dbc->bookChanged.side = ticker.bookChangedSide;
        dbc->bookChanged.timestamps.srcReceive = ticker.srcReceiveTime;
        dbc->bookChanged.timestamps.srcTransmit = ticker.srcTransmitTime;
        dbc->bookChanged.timestamps.producerReceive = ticker.producerReceiveTime;
        dbc->bookChanged.timestamps.producerTransmit = pHeader->transmitTimestamp = midas::ntime();
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

    uint8_t* pData = dataChannel->write_prepare(
        mdThreadNum, sizeof(Header) + sizeof(DataTradingAction));  // write directly into shared memory
    if (pData) {
        Header* pHeader = (Header*)pData;
        pHeader->session = publisher->session;
        pHeader->seq = mdSequence;
        pHeader->streamId = mdThreadNum;
        pHeader->count = 1;
        pHeader->size = sizeof(DataTradingAction);

        DataTradingAction dummy;
        DataTradingAction* dta = (DataTradingAction*)(pData + sizeof(Header));
        *dta = dummy;
        dta->locate = ticker.locate;
        memcpy(dta->symbol, ticker.mdTick.c_str(), ticker.mdTick.length());
        dta->exchange = exch;
        dta->tradingAction = ticker.tradingAction;
        dta->tradingAction.timestamps.srcReceive = ticker.srcReceiveTime;
        dta->tradingAction.timestamps.srcTransmit = ticker.srcTransmitTime;
        dta->tradingAction.timestamps.producerReceive = ticker.producerReceiveTime;
        dta->tradingAction.timestamps.producerTransmit = pHeader->transmitTimestamp = midas::ntime();
        dataChannel->write_commit(mdThreadNum, sizeof(Header) + sizeof(DataTradingAction));
    }
}

/*
 * DataSource->ConsumerProxy::send_data_heartbeat-><<<Shared Memory Queue>>>->DataChannel-on-Consumer
 */
void ConsumerProxy::send_data_heartbeat(uint8_t queueIndex, uint64_t stepSequence) {
    if (!(subscriptionFlags & FLAG_SEND_DATA_HEARTBEAT)) return;

    uint8_t* pData = dataChannel->write_prepare(queueIndex, sizeof(Header));  // write directly into shared memory
    if (pData) {
        Header* pHeader = (Header*)pData;
        pHeader->session = publisher->session;
        pHeader->seq = stepSequence;
        pHeader->streamId = queueIndex;
        pHeader->count = 0;
        pHeader->size = 0;  // size 0 okay here -- heartbeat indicator
        pHeader->transmitTimestamp = midas::ntime();
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
