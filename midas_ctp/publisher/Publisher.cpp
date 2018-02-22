#include <errno.h>
#include <fcntl.h>
#include <midas/md/MdDefs.h>
#include <midas/md/MdLock.h>
#include <net/raw/MdProtocol.h>
#include <net/raw/RawSocket.h>
#include <net/shm/SymbolData.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <boost/exception/diagnostic_information.hpp>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include "ConsumerProxy.h"
#include "DataChannel.h"
#include "DataSource.h"
#include "PubControlChannel.h"
#include "Publisher.h"

using namespace midas;

Publisher::Publisher(int argc, char** argv) : MidasProcessBase(argc, argv) {
    init_admin();
    startTime = midas::ntime();
    session = startTime;
}

Publisher::~Publisher() {}

void Publisher::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        THROW_MIDAS_EXCEPTION("failed to configure");
    }
}

void Publisher::app_stop() {}

bool Publisher::configure() {
    const string key{"ctp"};
    name = midas::get_cfg_value<string>(key, "name");
    {
        const string cacheKey{"ctp.book_cache"};
        create_book_cache(cacheKey);
    }
    return true;
}

void Publisher::init_admin() {
    admin_handler().register_callback("meters", boost::bind(&Publisher::admin_meters, this, _1, _2),
                                      "display statistical information of connections", "meters");
}

string Publisher::admin_meters(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    oss << setw(24) << "MD login time" << setw(24) << "MD logout time" << setw(24) << "Trade login time" << setw(24)
        << "Trade logout time\n";
    return oss.str();
}

void Publisher::create_book_cache(std::string const& key) {
    std::string exchangeKey = midas::get_cfg_value<string>(key, "exchange_key");
    std::string cacheName = midas::get_cfg_value<string>(key, "name");
    std::string symbolData = midas::get_cfg_value<string>(key, "symbol_data_file");

    if (exchangeKey.empty() || cacheName.empty() || symbolData.empty()) {
        throw std::runtime_error("exchange_key or name or symbol_data_file was not specified");
    }

    auto symDat = std::make_unique<SymbolData>(symbolData, SymbolData::UserFlag::readwrite);

    uint16_t exchangeCode = midas::get_cfg_value<uint16_t>(key, "exchange_code", std::numeric_limits<uint16_t>::max());
    vector<MdExchange> exchVec = DataSource::participant_exchanges();
    if (exchangeCode == std::numeric_limits<uint16_t>::max()) {
        std::ostringstream ostr;
        throw std::runtime_error("invalid exchange_key");
    }

    // verify cache does not yet exist for the exch
    if (bookCaches.count(exchangeCode)) {
        std::ostringstream ostr;
        ostr << "book_cache already created for " << exchangeKey << ":" << exchangeCode;
        throw std::runtime_error(ostr.str());
    }

    bookCaches.emplace(exchangeCode, std::make_unique<ShmCache>(key, exchVec, symDat.release()));
}
