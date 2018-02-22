#include "ShmCache.h"
#include "midas/MidasConfig.h"
#include "midas/md/MdBook.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {
ShmCache::ShmCache(std::string const& cachePropertyName, std::vector<MdExchange> const& exchanges,
                   SymbolData* symbolData_)
    : symbolData(symbolData_) {
    numProducts = get_cfg_value<uint16_t>(cachePropertyName, "num_products", UINT16_MAX);
    nameBookCache = get_cfg_value<string>(cachePropertyName, "name");
    MIDAS_LOG_INFO("num_products = " << numProducts << " name = " << nameBookCache);

    //
    // First size up the memory cache: sum up the depth for all exchanges
    //
    uint16_t totalDepth = 0;
    for (auto it = exchanges.begin(); it != exchanges.end(); ++it) {
        totalDepth = static_cast<uint16_t>(totalDepth + it->exchangeDepth);
    }

    //
    //---Layout of SHM book cache---
    //+-----------------------------------+<-- BEGIN BOOK METADATA
    //|   BookMetadata - EXCH1            |
    //|   ...                             |
    //|   ...                             |
    //|   ...                             |
    //|   BookMetadata - EXCHn            |
    //|-----------------------------------|
    //|   BookMetadata - dummy            |
    //|-----------------------------------|<-- BEGIN LOCATOR CODE 0 ----+
    //|   BidBookLevel1   - EXCH1         |                             |
    //|   BidBookLevel2   - EXCH1         |                             |
    //|   ...                             |                             |
    //|   BidBookLevelD_1 - EXCH1         |                             |
    //|-----------------------------------|                             |
    //|   ...                             |                             |
    //|   ...                             |                             |
    //|   ...                             |                             |
    //|-----------------------------------|                             |
    //|   BidBookLevel1   - EXCHn         |                             |
    //|   BidBookLevel2   - EXCHn         |                             |
    //|   ...                             |                             |
    //|   BidBookLevelD_N - EXCHn         |                             |
    //|-----------------------------------|                             |
    //|   BidBookLevel    - dummy         |                             |
    //|-----------------------------------|<-- BID LOCK (8 bytes)       |-> BYTES PER PRODUCT
    //|   BidBookLevel    - dummy         |                             |
    //|-----------------------------------|                             |
    //|   AskBookLevel1   - EXCH1         |                             |
    //|   AskBookLevel2   - EXCH1         |                             |
    //|   ...                             |                             |
    //|   AskBookLevelD_1 - EXCH1         |                             |
    //|-----------------------------------|                             |
    //|   ...                             |                             |
    //|   ...                             |                             |
    //|   ...                             |                             |
    //|-----------------------------------|                             |
    //|   AskBookLevel1   - EXCHn         |                             |
    //|   AskBookLevel2   - EXCHn         |                             |
    //|   ...                             |                             |
    //|   AskBookLevelD_N - EXCHn         |                             |
    //|-----------------------------------|                             |
    //|   AskBookLevel    - dummy         |                             |
    //|-----------------------------------|<-- ASK lock (8 bytes)       |
    //|   AskBookLevel    - dummy         |-----------------------------+
    //|-----------------------------------|
    //|   ...                             |
    //|   ...                             |<-- SHM OTHER PRODUCTS
    //|   ...                             |
    //|-----------------------------------|<-- BEGIN LOCATOR CODE <MAX>
    //|   BidBookLevel1   - EXCH1         |
    //|   BidBookLevel2   - EXCH1         |
    //|   ...                             |
    //|   BidBookLevelD_1 - EXCH1         |
    //|-----------------------------------|
    //|   ...                             |
    //|   ...                             |
    //|   ...                             |
    //|-----------------------------------|
    //|   BidBookLevel1   - EXCHn         |
    //|   BidBookLevel2   - EXCHn         |
    //|   ...                             |
    //|   BidBookLevelD_N - EXCHn         |
    //|-----------------------------------|
    //|   BidBookLevel    - dummy         |
    //|-----------------------------------|<-- BID LOCK (8 bytes)
    //|   BidBookLevel    - dummy         |
    //|-----------------------------------|
    //|   AskBookLevel1   - EXCH1         |
    //|   AskBookLevel2   - EXCH1         |
    //|   ...                             |
    //|   AskBookLevelD_1 - EXCH1         |
    //|-----------------------------------|
    //|   ...                             |
    //|   ...                             |
    //|   ...                             |
    //|-----------------------------------|
    //|   AskBookLevel1   - EXCHn         |
    //|   AskBookLevel2   - EXCHn         |
    //|   ...                             |
    //|   AskBookLevelD_N - EXCHn         |
    //|-----------------------------------|
    //|   AskBookLevel    - dummy         |
    //|-----------------------------------|<-- ASK LOCK (8 bytes)
    //|   AskBookLevel    - dummy         |
    //+-----------------------------------+

    /**
     * Alloc: BookMetaData for # of exchanges + 1 dummy, Book metadata at top of the shared memory cache
     * Bid side for all products, BidBookLevel for max num of symbols for total depth of all exchanges (+ 2 dummies)
     * Ask side for all products, AskBookLevel for max num of symbols for total depth of all exchanges (+ 2 dummies)
     */
    sizeBookCache = (exchanges.size() + 1) * sizeof(BookMetadata) +
                    (totalDepth + 2) * sizeof(BidBookLevel) * numProducts +
                    (totalDepth + 2) * sizeof(AskBookLevel) * numProducts;

    MIDAS_LOG_INFO("Sizing shared memory cache " << sizeBookCache);

    bool rc = true;
    fdBookCache = shm_open(nameBookCache.c_str(), O_CREAT | O_RDWR, 0666);
    if (fdBookCache >= 0) {
        if (ftruncate(fdBookCache, sizeBookCache) == 0) {
            struct stat stats;
            if (fstat(fdBookCache, &stats) == 0) {
                std::size_t size = static_cast<std::size_t>(stats.st_size);
                if (size >= sizeBookCache) {
                    sizeBookCache = size;
                    addrBookCache = mmap(nullptr, sizeBookCache, PROT_READ | PROT_WRITE, MAP_SHARED, fdBookCache, 0);
                    if (addrBookCache != MAP_FAILED) {
                        MIDAS_LOG_INFO("Book cache -- name: " << nameBookCache << ", fd: " << fdBookCache << ", size: "
                                                              << sizeBookCache << ", addr: " << addrBookCache);
                        struct rlimit limit;
                        if (getrlimit(RLIMIT_MEMLOCK, &limit) < 0) {
                            MIDAS_LOG_ERROR("getrlimit: " << errno << " " << strerror(errno));
                        } else {
                            MIDAS_LOG_INFO("RLIMIT_MEMLOCK - soft_limit: " << limit.rlim_cur
                                                                           << ", hard_limit: " << limit.rlim_max);
                        }
                        if (mlock(addrBookCache, sizeBookCache) < 0) {
                            MIDAS_LOG_ERROR("mlock: " << errno << " " << strerror(errno));
                        }
                    } else {
                        MIDAS_LOG_ERROR("mmap: " << errno << " " << strerror(errno));
                        rc = false;
                    }
                } else {
                    rc = false;
                }
            } else {
                MIDAS_LOG_ERROR("fstat: " << errno << " " << strerror(errno));
                rc = false;
            }
        } else {
            MIDAS_LOG_ERROR("ftruncate: " << errno << " " << strerror(errno));
            rc = false;
        }
    } else {
        MIDAS_LOG_ERROR("shm_open: " << errno << " " << strerror(errno));
        rc = false;
    }

    if (!rc) {
        munmap(addrBookCache, sizeBookCache);
        close(fdBookCache);
        fdBookCache = -1;
        shm_unlink(nameBookCache.c_str());
        throw std::runtime_error(strerror(errno));
    }

    auto numBytes = init_book_cache((uint8_t *) addrBookCache, exchanges);
    MIDAS_LOG_INFO(numBytes << " bytes initialized in shared memory book cache");
}

ShmCache::~ShmCache() {
    if (addrBookCache != MAP_FAILED) {
        MIDAS_LOG_INFO("Destroying shared memory book cache " << nameBookCache);
        munmap(addrBookCache, sizeBookCache);
        close(fdBookCache);
        fdBookCache = -1;
        shm_unlink(nameBookCache.c_str());
    }
}

std::size_t ShmCache::init_book_cache(uint8_t *startingAddress, std::vector<midas::MdExchange> const &exchanges) {
    std::size_t numOtherBytes = 0;
    BookMetadata dummy;
    BookMetadata* metadata = (BookMetadata*)startingAddress;
    {
        BookMetadata* metap = metadata;
        for (auto it = exchanges.begin(); it != exchanges.end(); ++it) {
            *metap = dummy;
            metap->exchange = it->exchange;
            metap->exchangeDepth = it->exchangeDepth;
            metap->exchangeOffsetBytesBid = it->offsetBytesBid;
            metap->exchangeOffsetBytesAsk = it->offsetBytesAsk;
            sizeBookMetadata += sizeof(BookMetadata);
            ++metap;
        }
        {
            // Now initialize the dummy part
            *metap = dummy;
            sizeBookMetadata += sizeof(BookMetadata);
            ++metap;
        }
    }

    // calculate the postMetadata address: this is the address the application will use:
    // the address returned from the address() method
    uint8_t* postMetadata = startingAddress + sizeBookMetadata;

    BidBookLevel dummyBid;
    AskBookLevel dummyAsk;

    for (uint16_t u = 0; u < numProducts; ++u) {
        BidBookLevel* bl = nullptr;
        for (BookMetadata* metap = metadata; metap->exchange != ExchangeNone; ++metap) {
            bl = (BidBookLevel*)(postMetadata + numOtherBytes);
            for (uint8_t d = 0; d < metap->exchangeDepth; ++d) {
                *bl = dummyBid;
                bl->exchange = metap->exchange;
                ++bl;
                numOtherBytes += sizeof(BidBookLevel);
            }
        }
        {
            // Now initialize the dummy bid level
            *bl = dummyBid;
            bl->exchange = ExchangeNone;
            ++bl;
            numOtherBytes += sizeof(BidBookLevel);
            uint64_t* lock = (uint64_t*)(postMetadata + numOtherBytes);
            *lock = 0;
            ++lock;
            *lock = 0xdeadbeefdeadbeef;
            numOtherBytes += sizeof(BidBookLevel);
        }
        AskBookLevel* al = nullptr;
        for (BookMetadata* metap = metadata; metap->exchange != ExchangeNone; ++metap) {
            al = (AskBookLevel*)(postMetadata + numOtherBytes);
            for (uint8_t d = 0; d < metap->exchangeDepth; ++d) {
                *al = dummyAsk;
                al->exchange = metap->exchange;
                ++al;
                numOtherBytes += sizeof(AskBookLevel);
            }
        }
        {
            // Now initialize the dummy ask level
            *al = dummyAsk;
            al->exchange = ExchangeNone;
            ++al;
            numOtherBytes += sizeof(AskBookLevel);
            uint64_t* lock = (uint64_t*)(postMetadata + numOtherBytes);
            *lock = 0;
            ++lock;
            *lock = 0xdeadbeefdeadbeef;
            numOtherBytes += sizeof(AskBookLevel);
        }
    }
    return (sizeBookMetadata + numOtherBytes);
}
}
