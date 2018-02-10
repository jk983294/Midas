#ifndef MIDAS_MD_PROTOCOL_H
#define MIDAS_MD_PROTOCOL_H

#include <sys/types.h>
#include <unistd.h>
#include <cstdint>

namespace midas {

#pragma pack(push, 1)

struct Timestamps {
    uint64_t srcReceive = 0;
    uint64_t srcTransmit = 0;
    uint64_t producerReceive = 0;
    uint64_t producerTransmit = 0;
};

struct BookChanged {
    enum struct ChangedSide : uint8_t { bid = 0, ask = 1, both = 2, none = 255 };
    ChangedSide side = ChangedSide::none;
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

    TradeStatus state = TradeStatus::none;
    Timestamps timestamps;
};

struct DataHeartbeat {
    uint64_t sessionId = 0;
    uint8_t streamId = 0;
    Timestamps timestamps;  // only producerTransmit is set
};

typedef void (*BookRefreshedHandler)(char const* symbol, uint16_t exch, void* userData);
typedef void (*BookChangedHandler)(char const* symbol, uint16_t exch, BookChanged const* bkchngd, void* userData);
typedef void (*TradingActionHandler)(char const* symbol, uint16_t exch, TradingAction const* trdst, void* userData);
typedef void (*SubscribeResponseHandler)(char const* symbol, uint16_t exch, int statusSubscription, void* userData);
typedef void (*ConnectHandler)(uint8_t status, void* userData);
typedef void (*DisconnectHandler)(uint8_t status, void* userData);
typedef void (*ClientPeriodicalHandler)(void* context_);
typedef void (*DataHeartbeatHandler)(DataHeartbeat const* dhb, void* userData);

struct MDConsumerCallbacks {
    BookRefreshedHandler cbBookRefreshed = nullptr;
    BookChangedHandler cbBookChanged = nullptr;
    TradingActionHandler cbTradingAction = nullptr;
    SubscribeResponseHandler cbSubscribeResponse = nullptr;
    ConnectHandler cbConnect = nullptr;
    DisconnectHandler cbDisconnect = nullptr;
    struct {
        ClientPeriodicalHandler cbClientPeriodical;
        void* cbClientContext;
        uint32_t cbInterval;
    } periodicalCB = {nullptr, nullptr, 1000};
    DataHeartbeatHandler cbDataHeartbeat = nullptr;
};

#pragma pack(pop)
}

#endif
