#ifndef MIDAS_MD_PROTOCOL_H
#define MIDAS_MD_PROTOCOL_H

#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
#include "midas/md/MdDefs.h"

namespace midas {

#pragma pack(push, 1)

struct Timestamps {
    uint64_t srcReceive{0};
    uint64_t srcTransmit{0};
    uint64_t producerReceive{0};
    uint64_t producerTransmit{0};
};

struct BookChanged {
    enum struct ChangedSide : uint8_t { bid = 0, ask = 1, both = 2, none = 255 };
    ChangedSide side{ChangedSide::none};
    Timestamps timestamps;
};

struct TradingAction {
    enum struct TradeStatus : uint8_t {
        auction = 0,
        auction_close = 1,
        auction_close2 = 2,
        auction_intraday = 3,
        auction_intraday2 = 4,
        auction_open = 5,
        auction_open2 = 6,
        auction_volatility = 7,
        auction_volatility2 = 8,
        closed = 9,
        halt = 10,
        halt_quoting = 11,
        obtrd = 12,
        popen = 13,
        postclose = 14,
        none = 255
    };

    TradeStatus state{TradeStatus::none};
    Timestamps timestamps;
};

struct Heartbeat {
    uint64_t sessionId{0};
    uint8_t streamId{0};
    Timestamps timestamps;  // only producerTransmit is set
};

typedef void (*BookRefreshedHandler)(char const* symbol, uint16_t exchange, void* userData);
typedef void (*BookChangedHandler)(char const* symbol, uint16_t exchange, BookChanged const* bkchngd, void* userData);
typedef void (*TradingActionHandler)(char const* symbol, uint16_t exchange, TradingAction const* trdst, void* userData);
typedef void (*SubscribeResponseHandler)(char const* symbol, uint16_t exchange, int statusSubscription, void* userData);
typedef void (*ConnectHandler)(uint8_t status, void* userData);
typedef void (*DisconnectHandler)(uint8_t status, void* userData);
typedef void (*ClientPeriodicalHandler)(void* context_);
typedef void (*DataHeartbeatHandler)(Heartbeat const* dhb, void* userData);

struct MDConsumerCallbacks {
    BookRefreshedHandler cbBookRefreshed{nullptr};
    BookChangedHandler cbBookChanged{nullptr};
    TradingActionHandler cbTradingAction{nullptr};
    SubscribeResponseHandler cbSubscribeResponse{nullptr};
    ConnectHandler cbConnect{nullptr};
    DisconnectHandler cbDisconnect{nullptr};
    struct {
        ClientPeriodicalHandler cbClientPeriodical;
        void* cbClientContext;
        uint32_t cbInterval;
    } periodicalCB = {nullptr, nullptr, 1000};
    DataHeartbeatHandler cbDataHeartbeat{nullptr};
};

#pragma pack(pop)

#pragma pack(push, 1)

/*
 * Control messages are exchanged between Publisher and Consumer over a TCP connection
 * which is established when Consumer starts up.
 * Data traffic may be segregated into multiple streams.
 * Each packet starts with a header. one packet contains multiple messages
 * Heartbeat exchanged only with an header
 */
struct Header {
    uint64_t session{0};  // session id
    uint64_t seq{0};      // sequence#
    uint64_t xmitts{0};   // transmit timestamp for message unit
    uint8_t streamId{0};  // stream-id - always 0 for control traffic
    uint8_t count{0};     // number of messages in 'message packet'
    uint16_t size{0};     // total size (bytes) of messages (excluding header size) in 'message packet'

    Header() = default;
};

struct MsgBase {
    uint8_t type{0};
    uint16_t msgSize{sizeof(MsgBase)};

    MsgBase() = default;
};

struct CtrlConnect {
    uint8_t type = CTRL_CONNECT_TYPE;
    uint16_t msgSize{sizeof(CtrlConnect)};
    pid_t clientPid{0};
    uint8_t clientId{0};
    uint32_t flags{0};  // Indicates event types mdc is interested in
    char user[9]{};
    char pwd[13]{};
    char shmKey[18]{};

    CtrlConnect() = default;
};

struct CtrlConnectResponse {
    uint8_t type{CTRL_CONNECT_RESPONSE_TYPE};
    uint16_t msgSize{sizeof(CtrlConnectResponse)};
    uint8_t cid{0};
    uint8_t status{0};      // 0 => OK, 'D' => Client Already Connected, 'V' => Version Mismatch
    uint8_t shmType{0x00};  // 'E' => event queue, 'B' => book cache, 0 => ignore
    char shmPath[18]{};
    uint32_t shmSize{0};
    uint16_t exchange{ExchangeNone};

    CtrlConnectResponse() = default;
};

struct CtrlDisconnect {
    uint8_t type{CTRL_DISCONNECT_TYPE};
    uint16_t msgSize{sizeof(CtrlDisconnect)};
    pid_t cpid{0};

    CtrlDisconnect() = default;
};

struct CtrlDisconnectResponse {
    uint8_t type{CTRL_DISCONNECT_RESPONSE_TYPE};
    uint16_t msgSize{sizeof(CtrlDisconnectResponse)};
    uint8_t status{0};

    CtrlDisconnectResponse() = default;
};

struct CtrlSubscribe {
    uint8_t type{CTRL_SUBSCRIBE_TYPE};
    uint16_t msgSize{sizeof(CtrlSubscribe)};
    char symbol[9]{};
    uint16_t exchange{ExchangeNone};  // the exchange for the subscription
    uint32_t flags{0};                // Indicates event types mdc is interested in

    CtrlSubscribe() = default;
};

struct CtrlSubscribeResponse {
    uint8_t type{CTRL_SUBSCRIBE_RESPONSE_TYPE};
    uint16_t msgSize{sizeof(CtrlSubscribeResponse)};
    uint8_t status{0};
    uint16_t locate{0};
    char symbol[9]{};
    uint16_t exchange{ExchangeNone};  // the exchange for the symbol

    CtrlSubscribeResponse() = default;
};

struct CtrlUnsubscribe {
    uint8_t type{CTRL_UNSUBSCRIBE_TYPE};
    uint16_t msgSize{sizeof(CtrlUnsubscribe)};
    char symbol[9]{};
    uint16_t exchange{ExchangeNone};

    CtrlUnsubscribe() = default;
};

struct CtrlUnsubscribeResponse {
    uint8_t type{CTRL_UNSUBSCRIBE_RESPONSE_TYPE};
    uint16_t msgSize{sizeof(CtrlUnsubscribeResponse)};
    uint8_t status{0};
    uint16_t locate{0};
    char symbol[9]{};
    uint16_t exchange{ExchangeNone};

    CtrlUnsubscribeResponse() = default;
};

struct DataTradingAction {
    uint8_t type{DATA_TRADING_ACTION_TYPE};
    uint16_t msgSize{sizeof(DataTradingAction)};
    uint64_t pubrcvt{0};  // MDPublisher receive timestamp
    uint16_t locate{0};
    char symbol[9]{};
    uint16_t exchange{ExchangeNone};  // exchange where action occurred
    TradingAction tradingAction;

    DataTradingAction() = default;
};

struct DataBookChanged {
    uint8_t type{DATA_BOOK_CHANGED_TYPE};
    uint16_t msgSize{sizeof(DataBookChanged)};
    uint64_t pubrcvt{0};  // MDPublisher receive timestamp
    uint16_t locate{0};
    char symbol[9]{};
    uint16_t exchange{ExchangeNone};  // the exchange where the event occurred
    BookChanged bookchanged;

    DataBookChanged() = default;
};

struct DataBookRefreshed {
    uint8_t type{DATA_BOOK_REFRESHED_TYPE};
    uint16_t msgSize{sizeof(DataBookRefreshed)};
    uint64_t pubrcvt{0};  // MDPublisher receive timestamp
    uint16_t locate{0};
    char symbol[9]{};
    uint16_t exchange{ExchangeNone};  // the exchange where the event occurred

    DataBookRefreshed() = default;
};

struct DataHeartbeat {
    uint8_t type{DATA_HEARTBEAT_TYPE};
    uint16_t msgSize{sizeof(DataHeartbeat)};
    Heartbeat hb;

    DataHeartbeat() = default;
};

struct AdminBookClear {
    uint8_t type{ADMIN_BOOK_CLEAR_TYPE};
    uint16_t msgSize{sizeof(AdminBookClear)};
    void* ticker{nullptr};

    AdminBookClear() = default;
};

struct MonPublisherInfo {
    uint8_t type{MON_PUBLISHER_INFO_TYPE};
    uint16_t msgSize{sizeof(MonPublisherInfo)};
    char name[32]{};
    char version[16]{};
    char host[32]{};
    pid_t pid{0};
    uint64_t sessionid{0};
    uint64_t starttm{0};
    uint8_t numconsumers{0};

    MonPublisherInfo() = default;
};

struct MonConsumerInfo {
    uint8_t type{MON_CONSUMER_INFO_TYPE};
    uint16_t msgSize{sizeof(MonConsumerInfo)};
    char name[32]{};
    uint8_t id{0};
    pid_t pid{0};
    char user[32]{};
    char ipaddr[32]{};
    uint16_t port = UINT16_MAX;
    uint64_t connecttm{0};
    uint32_t numsubs{0};

    MonConsumerInfo() = default;
};

struct MonDataChannelInfo {
    uint8_t type{MON_DATACHANNEL_INFO_TYPE};
    uint16_t msgSize{sizeof(MonDataChannelInfo)};
    char name[32]{};
    uint8_t id{0};
    uint64_t readmsgs{0};
    uint64_t readbytes{0};
    uint64_t writtenmsgs{0};
    uint64_t writtenbytes{0};
    uint64_t droppedmsgs{0};
    uint64_t droppedbytes{0};

    MonDataChannelInfo() = default;
};

#pragma pack(pop)
}

#endif
