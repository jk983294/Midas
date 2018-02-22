#include "SymbolData.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {
__data::__data(std::string const& symbol, uint16_t locator) : locator_shm(locator) { set_symbol(this, symbol); }

__data::__vsd* get_vsd(__data* d, uint16_t e) {
    for (std::size_t i = 0; i < sizeof(d->vsd) / sizeof(d->vsd[0]); ++i) {
        __data::__vsd& v = d->vsd[i];
        auto exch = get_exchange(&v);
        if (exch == e) {
            return &v;
        }
        if (!exch) {
            set_exchange(&v, e);  // claim this spot
            return &v;
        }
    }

    return nullptr;
}

std::ostream& operator<<(std::ostream& o, __data const& d) {
    bool x = false;

    for (std::size_t i = 0; i < sizeof(d.vsd) / sizeof(d.vsd[0]); ++i) {
        __data::__vsd const& v = d.vsd[i];
        if (get_exchange(&v)) {
            o << get_symbol(&d) << "," << get_locator(&d) << "," << get_in_use(&d) << "," << get_new_symbol(&d) << ","
              << get_exchange(&v) << "," << get_max_bid_depth(&v) << "," << get_max_ask_depth(&v) << "\n";
            if (!x) x = true;
        }
    }

    if (!x) {
        o << get_symbol(&d) << "," << get_locator(&d) << "," << get_in_use(&d) << "," << get_new_symbol(&d)
          << ",0,0,0\n";
    }
    return o;
}

SymbolData::SymbolData(std::string const& symbolFile, UserFlag flag) : userFlag(flag) {
    pageSize = static_cast<size_t>(::sysconf(_SC_PAGESIZE));
    try {
        int oflag = 0;
        if (userFlag == UserFlag::readwrite) {
            oflag |= (O_CREAT | O_RDWR | O_CLOEXEC);
        } else {
            oflag |= (O_RDONLY | O_CLOEXEC);
        }

        fd = open(symbolFile.c_str(), oflag, 0644);
        if (fd != -1) {
            this->mmap(nullptr);
            init();
        } else {
            std::ostringstream ostr;
            ostr << "open: " << symbolFile << ",  errno " << errno;
            throw std::runtime_error(ostr.str());
        }
    } catch (std::runtime_error const& err) {
        MIDAS_LOG_ERROR(err.what());
        finis();
        throw;
    }
}

bool SymbolData::locate(std::string const& symbol, uint16_t& locator, bool create) {
    std::lock_guard<std::recursive_mutex> g(mutex);

    auto it = lookupBySymbol.find(symbol);
    if (it != lookupBySymbol.end()) {
        locator = get_locator(it->second);
        return true;
    } else if (create) {
        // symbol does not exist but we are allowed to create one
        add_symbol(symbol, locator, true);
        return true;
    }
    return false;
}

bool SymbolData::locate(uint16_t locator, std::vector<__data*>& symbolData) {
    std::lock_guard<std::recursive_mutex> g(mutex);
    auto itPair = lookupByLocator.equal_range(locator);
    for (auto locIt = itPair.first; locIt != itPair.second; ++locIt) {
        symbolData.emplace_back(locIt->second);
    }
    return !symbolData.empty();
}

bool SymbolData::copy_locate(std::string const& sourceSymbol, std::string const& targetSymbol, uint16_t& locator) {
    std::lock_guard<std::recursive_mutex> g(mutex);

    auto symIt1 = lookupBySymbol.find(sourceSymbol);
    auto symIt2 = lookupBySymbol.find(targetSymbol);
    if (symIt1 != lookupBySymbol.end()) {  // source symbol exists
        auto sourceLocator = get_locator(symIt1->second);
        if (symIt2 != lookupBySymbol.end()) {  // target symbol exists
            modify_symbol(targetSymbol, sourceLocator);
        } else {
            add_symbol(targetSymbol, sourceLocator, false);  // must use sourceLocator
        }
        locator = sourceLocator;
        return true;
    } else {  // source symbol does not exist
        return locate(targetSymbol, locator, true);
    }
}

bool SymbolData::remove(std::string const& symbol) {
    std::lock_guard<std::recursive_mutex> g(mutex);
    auto symIt = lookupBySymbol.find(symbol);
    if (symIt != lookupBySymbol.end()) {
        delete_symbol(symbol);
        return true;
    }
    return false;
}

std::ostream& operator<<(std::ostream& o, SymbolData const& sd) {
    uint32_t count = 0;
    for (__data *iterator = sd.first; count < sd.hdr->capacityShm; ++count, ++iterator) {
        if (get_in_use(iterator)) {
            o << *iterator;
        }
    }
    return o;
}

void SymbolData::add_symbol(std::string const& symbol, uint16_t& locator, bool assignFreeLocator) {
    if (userFlag == UserFlag::readonly) {
        throw std::runtime_error("add_symbol: only a writer may add symbol");
    }

    if (lookupBySymbol.find(symbol) != lookupBySymbol.end()) {
        std::ostringstream ostr;
        ostr << "add_symbol: " << symbol << " already exists!";
        throw std::runtime_error(ostr.str());
    }

    if (hdr->countShm == hdr->capacityShm) {
        throw std::runtime_error("add_symbol: max capacity breaching");
    }

    __data* here = pool.front();
    pool.pop_front();  // pool must have free entry available here
    if (assignFreeLocator) {
        auto freeLocator = get_locator(here);  // see if there is a locator code available for reuse
        if (freeLocator == SYMBOL_DATA_INVALID_LOCATOR_CODE) {
            if (generator == maxLocator) {
                throw std::runtime_error("add_symbol: max locator code breaching");
            }
            freeLocator = ++generator;
        } else {
            if (lookupByLocator.count(freeLocator) > 0) {  // is this locator free for use?
                if (generator == maxLocator) {
                    throw std::runtime_error("add_symbol: max locator code breaching");
                }
                freeLocator = ++generator;
            }
        }
        locator = freeLocator;
    }
    __data d(symbol, locator);
    set_newsymbol(&d, true);  // mark as new symbol
    set_in_use(&d, true);     // mark as this symbol entry being in-use
    *here = d;                // copy to mmap
    hdr->countShm += 1;       // increment element count
    add_internal(here);       // add to internal map
    ::msync(hdr, sizeof(__header), MS_ASYNC);
    ::msync(page_aligned(here), pageSize, MS_ASYNC);
    MIDAS_LOG_INFO("Added new symbol " << symbol << " with locator code " << locator);
    return;
}

void SymbolData::add_internal(__data* d) {
    auto symbol(get_symbol(d));
    auto locator(get_locator(d));
    lookupBySymbol[symbol] = d;
    lookupByLocator.insert(std::make_pair(locator, d));
    if (locator > generator) {
        generator = locator;  // move up the generator
    }
}

void SymbolData::modify_symbol(std::string const& symbol, uint16_t locator) {
    if (userFlag == UserFlag::readonly) {
        throw std::runtime_error("modify_symbol: only a writer may modify symbol");
    }

    auto symIt = lookupBySymbol.find(symbol);
    if (symIt != lookupBySymbol.end()) {
        __data* here = symIt->second;
        set_locator(here, locator);
        mod_internal(here);  // modify internal map
        ::msync(page_aligned(here), pageSize, MS_ASYNC);
        MIDAS_LOG_INFO("Modified symbol " << symbol << " to locator " << locator);
    } else {
        std::ostringstream ostr;
        ostr << "modify_symbol: " << symbol << " does not exist!";
        std::runtime_error(ostr.str());
    }
}

void SymbolData::mod_internal(__data* d) {
    auto symbol(get_symbol(d));
    auto symIt = lookupBySymbol.find(symbol);
    if (symIt != lookupBySymbol.end()) {
        auto prevLocator = get_locator(symIt->second);
        auto result = lookupByLocator.equal_range(prevLocator);
        for (auto it = result.first; it != result.second; ++it) {
            if (get_symbol(it->second) == symbol) {
                lookupByLocator.erase(it);
                break;
            }
        }
        lookupBySymbol.erase(symIt);
    }
    lookupBySymbol[symbol] = d;

    bool found = false;
    auto newLocator(get_locator(d));
    auto result = lookupByLocator.equal_range(newLocator);
    for (auto it = result.first; it != result.second; ++it) {
        if (get_symbol(it->second) == symbol) {
            it->second = d;
            found = true;
            break;
        }
    }
    if (!found) {
        lookupByLocator.insert(std::make_pair(newLocator, d));
    }
}

void SymbolData::delete_symbol(std::string const& symbol) {
    if (userFlag == UserFlag::readonly) {
        throw std::runtime_error("delete_symbol: only a writer may delete symbol");
    }

    auto symIt = lookupBySymbol.find(symbol);
    if (symIt != lookupBySymbol.end()) {   //'symbol' found
        set_in_use(symIt->second, false);  // mark in mmapped area
        ::msync(page_aligned(symIt->second), pageSize, MS_ASYNC);
        pool.push_back(symIt->second);  // return back to pool for reuse
        del_internal(symIt->second);    // clear internal maps
        hdr->countShm -= 1;             // decrement element count
        ::msync(hdr, sizeof(__header), MS_ASYNC);
    } else {
        std::ostringstream ostr;
        ostr << "delete_symbol: " << symbol << " does not exist!";
        throw std::runtime_error(ostr.str());
    }
}

void SymbolData::del_internal(__data* d) {
    auto symbol(get_symbol(d));
    auto locator(get_locator(d));

    auto symIt = lookupBySymbol.find(symbol);
    if (symIt != lookupBySymbol.end()) {
        lookupBySymbol.erase(symIt);
    }

    auto result = lookupByLocator.equal_range(locator);
    for (auto it = result.first; it != result.second; ++it) {
        if (get_symbol(it->second) == symbol) {
            lookupByLocator.erase(it);
            break;
        }
    }
}

void SymbolData::clr_internal() {
    lookupBySymbol.clear();
    lookupByLocator.clear();
    pool.clear();
    generator = 0;
}

void SymbolData::mmap(void* address) {
    std::ostringstream ostr;
    struct stat stats;
    if (fstat(fd, &stats) < 0) {
        ostr << "fstat: errno " << errno;
        throw std::runtime_error(ostr.str());
    } else {
        std::size_t currsz = static_cast<std::size_t>(stats.st_size);
        if ((mappedSize != currsz) && userFlag == UserFlag::readwrite && ftruncate(fd, mappedSize) < 0) {
            ostr << "ftruncate: errno " << errno;
            throw std::runtime_error(ostr.str());
        }

        if (userFlag == UserFlag::readonly) {
            mappedSize = currsz;
        }

        int prot = (userFlag == UserFlag::readwrite ? PROT_READ | PROT_WRITE : PROT_READ);
        addr = ::mmap(address, mappedSize, prot, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            ostr << "mmap: errno " << errno;
            throw std::runtime_error(ostr.str());
        } else {
            hdr = (__header*)addr;
            if ((hdr->capacityShm != symbolCapacity) && (userFlag == UserFlag::readwrite)) {
                hdr->capacityShm = symbolCapacity;
                ::msync(hdr, sizeof(__header), MS_ASYNC);
            }

            if (userFlag == UserFlag::readonly) {
                symbolCapacity = hdr->capacityShm;
            }
        }
    }
}

void SymbolData::init() {
    std::lock_guard<std::recursive_mutex> g(mutex);
    clr_internal();
    hdr = (__header*)addr;
    first = (__data*)((char*)hdr + sizeof(hdr));

    uint32_t count = 0;
    for (__data *iterator = first; count < hdr->capacityShm; ++count, ++iterator) {
        if (get_in_use(iterator)) {
            add_internal(iterator);
            if (userFlag == UserFlag::readwrite && get_new_symbol(iterator)) {
                set_newsymbol(iterator, false);  // reset if this was marked new before
            }
        } else {
            if (userFlag == UserFlag::readwrite && get_locator(iterator) == 0) {
                __data d;
                *iterator = d;  // initialize memory
            }
            pool.push_back(iterator);
        }
    }

    if (userFlag == UserFlag::readwrite) {
        ::msync(addr, mappedSize, MS_ASYNC);
    }
    MIDAS_LOG_ERROR("count: " << hdr->countShm << ", capacity: " << hdr->capacityShm);
}

void SymbolData::finis() {
    std::lock_guard<std::recursive_mutex> g(mutex);
    if (addr != MAP_FAILED) {
        if (userFlag == UserFlag::readwrite) {
            ::msync(addr, mappedSize, MS_ASYNC);
        }
        ::munmap(addr, mappedSize);
        addr = MAP_FAILED;
    }

    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
    clr_internal();
}
}
