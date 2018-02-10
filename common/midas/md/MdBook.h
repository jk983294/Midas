#ifndef MIDAS_MD_BOOK_H
#define MIDAS_MD_BOOK_H

#include <cstdint>
#include <memory>
#include "MdDefs.h"

namespace midas {

#pragma pack(push, 1)

struct BookMetadata {
    uint8_t watermark[8] = {'b', '0', '0', 'k', 'M', 'e', 'T', 'A'};
    uint16_t exchange = ExchangeNone;
    uint8_t exchangeDepth = 0;
    uint16_t exchangeOffsetBytesBid = 0;
    uint16_t exchangeOffsetBytesAsk = 0;
};

struct BidBookLevel {
    int64_t price = PriceBlank;
    uint64_t shares = 0;
    uint32_t orders = 0;
    uint16_t exchange = ExchangeNone;
    uint64_t sequence = 0;
    uint64_t updateTS = 0;
    uint64_t timestamp = 0;
    uint8_t priceScaleCode = DefaultPriceScale;
    uint32_t lotSize = DefaultLotSize;
};

struct AskBookLevel {
    int64_t price = PriceBlank;
    uint64_t shares = 0;
    uint32_t orders = 0;
    uint16_t exchange = ExchangeNone;
    uint64_t sequence = 0;
    uint64_t updateTS = 0;
    uint64_t timestamp = 0;
    uint8_t priceScaleCode = DefaultPriceScale;
    uint32_t lotSize = DefaultLotSize;
};

struct MdBook {
    enum struct BookType : uint8_t { price = 0, image = 1, order = 2, none = 255 };

    void* __thunk = nullptr;
    uint16_t numBidLevels = 0;
    BidBookLevel* bidLevels = nullptr;
    uint16_t numAskLevels = 0;
    AskBookLevel* askLevels = nullptr;
    BookType bookType = BookType::none;
    uint8_t myAlloc = 1;

    MdBook() = default;

    ~MdBook() {
        if (myAlloc) {
            delete[] bidLevels;
            delete[] askLevels;
        }
    }

    MdBook(MdBook const&) = delete;
    MdBook& operator=(MdBook const&) = delete;
};

#pragma pack(pop)

inline void clearBidLevel(BidBookLevel* lp) {
    lp->price = PriceBlank;
    lp->priceScaleCode = DefaultPriceScale;
    lp->shares = 0;
    lp->orders = 0;
    lp->sequence = 0;
    lp->updateTS = 0;
    lp->timestamp = 0;
}

inline void clearAskLevel(AskBookLevel* lp) {
    lp->price = PriceBlank;
    lp->priceScaleCode = DefaultPriceScale;
    lp->shares = 0;
    lp->orders = 0;
    lp->sequence = 0;
    lp->updateTS = 0;
    lp->timestamp = 0;
}

inline bool operator<(BidBookLevel const& lhs, BidBookLevel const& rhs) { return lhs.price > rhs.price; }

inline bool operator<(BidBookLevel const& bidBookLevel, int64_t price) { return bidBookLevel.price > price; }

inline bool operator<(int64_t price, BidBookLevel const& bidBookLevel) { return price > bidBookLevel.price; }

inline bool operator>(BidBookLevel const& lhs, BidBookLevel const& rhs) { return lhs.price < rhs.price; }

inline bool operator>(BidBookLevel const& bidBookLevel, int64_t price) { return bidBookLevel.price < price; }

inline bool operator>(int64_t price, BidBookLevel const& bidBookLevel) { return price < bidBookLevel.price; }

inline bool operator<(std::unique_ptr<BidBookLevel> const& bidBookLevel, int64_t price) {
    return *bidBookLevel < price;
}

inline bool operator<(int64_t price, std::unique_ptr<BidBookLevel> const& bidBookLevel) {
    return price < *bidBookLevel;
}

inline bool operator>(std::unique_ptr<BidBookLevel> const& bidBookLevel, int64_t price) {
    return *bidBookLevel > price;
}

inline bool operator>(int64_t price, std::unique_ptr<BidBookLevel> const& bidBookLevel) {
    return price > *bidBookLevel;
}

inline bool operator<(AskBookLevel const& lhs, AskBookLevel const& rhs) { return lhs.price < rhs.price; }

inline bool operator<(AskBookLevel const& askBookLevel, int64_t price) { return askBookLevel.price < price; }

inline bool operator<(int64_t price, AskBookLevel const& askBookLevel) { return price < askBookLevel.price; }

inline bool operator>(AskBookLevel const& lhs, AskBookLevel const& rhs) { return lhs.price > rhs.price; }

inline bool operator>(AskBookLevel const& askBookLevel, int64_t price) { return askBookLevel.price > price; }

inline bool operator>(int64_t price, AskBookLevel const& askBookLevel) { return price > askBookLevel.price; }

inline bool operator<(std::unique_ptr<AskBookLevel> const& askBookLevel, int64_t price) {
    return *askBookLevel < price;
}

inline bool operator<(int64_t price, std::unique_ptr<AskBookLevel> const& askBookLevel) {
    return price < *askBookLevel;
}

inline bool operator>(std::unique_ptr<AskBookLevel> const& askBookLevel, int64_t price) {
    return *askBookLevel > price;
}

inline bool operator>(int64_t price, std::unique_ptr<AskBookLevel> const& askBookLevel) {
    return price > *askBookLevel;
}
}

#endif
