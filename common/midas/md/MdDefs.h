#ifndef MIDAS_MDDEFS_H
#define MIDAS_MDDEFS_H

#include <cstdint>

namespace midas {
constexpr uint16_t ExchangeNone = 0x0000;
constexpr uint16_t ExchangeCFFEX = 0x0001;  // 中国金融交易所
constexpr uint16_t ExchangeCZCE = 0x0002;   // 郑州商品交易所
constexpr uint16_t ExchangeDCE = 0x0003;    // 大连商品交易所
constexpr uint16_t ExchangeINE = 0x0004;    // 上海国际能源交易中心股份有限公司
constexpr uint16_t ExchangeSHFE = 0x0005;   // 上海期货交易所
constexpr uint16_t ExchangeCtpAll = 0x0006;

constexpr int64_t PriceBlank = INT64_MIN;
constexpr uint8_t DefaultPriceScale = 4;
constexpr uint32_t DefaultLotSize = 100;
constexpr int MdLockMetaOffset = 12;

constexpr uint8_t CTRL_CONNECT_TYPE = 'I';
constexpr uint8_t CTRL_CONNECT_RESPONSE_TYPE = 'Z';
constexpr uint8_t CTRL_DISCONNECT_TYPE = 'B';
constexpr uint8_t CTRL_DISCONNECT_RESPONSE_TYPE = 'Y';
constexpr uint8_t CTRL_SUBSCRIBE_TYPE = 'C';
constexpr uint8_t CTRL_SUBSCRIBE_RESPONSE_TYPE = 'X';
constexpr uint8_t CTRL_UNSUBSCRIBE_TYPE = 'D';
constexpr uint8_t CTRL_UNSUBSCRIBE_RESPONSE_TYPE = 'V';
constexpr uint8_t DATA_TRADING_ACTION_TYPE = 'E';
constexpr uint8_t DATA_BOOK_CHANGED_TYPE = 'G';
constexpr uint8_t DATA_BOOK_REFRESHED_TYPE = 'H';
constexpr uint8_t DATA_HEARTBEAT_TYPE = 'J';
constexpr uint8_t ADMIN_BOOK_CLEAR_TYPE = 'a';
constexpr uint8_t MON_PUBLISHER_INFO_TYPE = 'z';
constexpr uint8_t MON_CONSUMER_INFO_TYPE = 'y';
constexpr uint8_t MON_DATACHANNEL_INFO_TYPE = 'x';

constexpr uint8_t STREAM_ID_CONTROL_CHANNEL = 0;

constexpr uint8_t CONNECT_STATUS_OK = 0;
constexpr uint8_t CONNECT_STATUS_ALREADY_CONNECTED = 'D';
constexpr uint8_t CONNECT_STATUS_UNREGISTERED_CONNECTION = 'U';
constexpr uint8_t CONNECT_STATUS_SHARED_MEMORY_FAILURE = 'S';
constexpr uint8_t CONNECT_STATUS_INVALID_ID = 'I';
constexpr uint8_t CONNECT_STATUS_VERSION_MISMATCH = 'V';

constexpr uint8_t SUBSCRIBE_STATUS_OK = 0;
constexpr uint8_t SUBSCRIBE_STATUS_BAD_SYMBOL = 2;
constexpr uint8_t SUBSCRIBE_STATUS_BAD_EXCHANGE = 3;

constexpr uint8_t BOOK_SIDE_BID = 0;
constexpr uint8_t BOOK_SIDE_ASK = 1;
constexpr uint8_t BOOK_SIDE_BOTH = 2;

constexpr uint8_t CONSUMER_OK = 0;
constexpr uint8_t CONSUMER_NOT_CONNECTED = 1;
constexpr uint8_t CONSUMER_NOT_AUTHORIZED = 2;
constexpr uint8_t CONSUMER_BAD_SYMBOL = 3;
constexpr uint8_t CONSUMER_ERROR = 4;
constexpr uint8_t CONSUMER_SLOW_CONSUMER = 5;
constexpr uint8_t CONSUMER_SHM_ERROR = 6;
constexpr uint8_t CONSUMER_TIMED_OUT = 7;
constexpr uint8_t CONSUMER_BAD_BOOK = 8;
constexpr uint8_t CONSUMER_NOT_SUBSCRIBED = 9;
constexpr uint8_t CONSUMER_NO_DATA = 10;
constexpr uint8_t CONSUMER_ALREADY_SUBSCRIBED = 11;

constexpr uint8_t SHM_TYPE_EVENT_QUEUE = 'E';
constexpr uint8_t SHM_TYPE_BOOK_CACHE = 'B';

constexpr uint32_t FLAG_SEND_BOOK_REFRESHED = 0x00000001;
constexpr uint32_t FLAG_SEND_BOOK_CHANGED = 0x00000002;
constexpr uint32_t FLAG_SEND_TRADING_ACTION = 0x00000004;
constexpr uint32_t FLAG_SEND_IMBALANCE = 0x00000008;
constexpr uint32_t FLAG_SEND_TRADE = 0x00000010;
constexpr uint32_t FLAG_SEND_QUOTE = 0x00000020;
constexpr uint32_t FLAG_SEND_DATA_HEARTBEAT = 0x00000040;

constexpr const char* BOOK_WATERMARK = "bookMeTA";
}

#endif
