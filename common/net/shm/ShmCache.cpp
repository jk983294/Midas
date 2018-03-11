#include "ShmCache.h"
#include "midas/MidasConfig.h"
#include "midas/md/MdBook.h"

using namespace std;

namespace midas {
ShmCache::ShmCache(std::string const& cachePropertyName, midas::MdExchange const& exchange, SymbolData* symbolData_)
    : symbolData(symbolData_) {
    numProducts = get_cfg_value<uint16_t>(cachePropertyName, "num_products", UINT16_MAX);
    nameBookCache = get_cfg_value<string>(cachePropertyName, "shmName");
    MIDAS_LOG_INFO("num_products = " << numProducts << " shmName = " << nameBookCache);

    uint16_t totalDepth = exchange.exchangeDepth;

    //
    //---Layout of SHM book cache---
    //+-----------------------------------+<-- BEGIN BOOK METADATA
    //|   BookMetadata - EXCH1            |
    //|-----------------------------------|
    //|   BookMetadata - dummy            |
    //|-----------------------------------|<-- BEGIN LOCATOR CODE 0 ----+
    //|   BidBookLevel1   - EXCH1         |                             |
    //|   BidBookLevel2   - EXCH1         |                             |
    //|   ...                             |                             |
    //|   BidBookLevelD_1 - EXCH1         |                             |
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
    //|   BidBookLevel    - dummy         |
    //|-----------------------------------|<-- BID LOCK (8 bytes)
    //|   BidBookLevel    - dummy         |
    //|-----------------------------------|
    //|   AskBookLevel1   - EXCH1         |
    //|   AskBookLevel2   - EXCH1         |
    //|   ...                             |
    //|   AskBookLevelD_1 - EXCH1         |
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
    sizeBookCache = 2 * sizeof(BookMetadata) + (totalDepth + 2) * sizeof(BidBookLevel) * numProducts +
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
                        MIDAS_LOG_INFO("Book cache -- shmName: /dev/shm/" << nameBookCache << ", fd: " << fdBookCache
                                                                          << ", size: " << sizeBookCache
                                                                          << ", addr: " << addrBookCache);
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

    auto numBytes = init_book_cache((uint8_t*)addrBookCache, exchange);
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

std::size_t ShmCache::init_book_cache(uint8_t* startingAddress, midas::MdExchange const& exchange) {
    BookMetadata dummy;
    BookMetadata* metadata = (BookMetadata*)startingAddress;
    {
        BookMetadata* pMeta = metadata;
        {
            *pMeta = dummy;
            pMeta->exchange = exchange.exchange;
            pMeta->exchangeDepth = exchange.exchangeDepth;
            pMeta->exchangeOffsetBytesBid = exchange.offsetBytesBid;
            pMeta->exchangeOffsetBytesAsk = exchange.offsetBytesAsk;
            sizeBookMetadata += sizeof(BookMetadata);
            ++pMeta;
        }
        {
            // Now initialize the dummy part, only one dummy metadata entry
            *pMeta = dummy;
            sizeBookMetadata += sizeof(BookMetadata);
        }
    }

    /**
     * calculate the postMetadata address:
     * this is the address the application will use: the address returned from the address() method
     */
    uint8_t* postMetadata = startingAddress + sizeBookMetadata;
    std::size_t numOtherBytes = 0;

    BidBookLevel dummyBid;
    AskBookLevel dummyAsk;

    for (uint16_t u = 0; u < numProducts; ++u) {
        BidBookLevel* bl = (BidBookLevel*)postMetadata;
        for (BookMetadata* pMeta = metadata; pMeta->exchange != ExchangeNone; ++pMeta) {
            bl = (BidBookLevel*)(postMetadata + numOtherBytes);
            for (uint8_t d = 0; d < pMeta->exchangeDepth; ++d) {
                *bl = dummyBid;
                bl->exchange = pMeta->exchange;
                ++bl;
                numOtherBytes += sizeof(BidBookLevel);
            }
        }
        {
            // Now initialize the dummy bid level, two dummy bid level, one for separator, one for lock
            *bl = dummyBid;
            bl->exchange = ExchangeNone;
            numOtherBytes += sizeof(BidBookLevel);
            uint64_t* lock = (uint64_t*)(postMetadata + numOtherBytes);
            *lock = 0;
            ++lock;
            *lock = 0xdeadbeefdeadbeef;
            numOtherBytes += sizeof(BidBookLevel);
        }

        AskBookLevel* al = (AskBookLevel*)postMetadata;
        for (BookMetadata* pMeta = metadata; pMeta->exchange != ExchangeNone; ++pMeta) {
            al = (AskBookLevel*)(postMetadata + numOtherBytes);
            for (uint8_t d = 0; d < pMeta->exchangeDepth; ++d) {
                *al = dummyAsk;
                al->exchange = pMeta->exchange;
                ++al;
                numOtherBytes += sizeof(AskBookLevel);
            }
        }
        {
            // Now initialize the dummy ask level, two dummy ask level, one for separator, one for lock
            *al = dummyAsk;
            al->exchange = ExchangeNone;
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
