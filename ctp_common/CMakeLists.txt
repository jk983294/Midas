set(SRCS
        dao/CandleDao.cpp
        dao/InstrumentInfoDao.cpp
        helper/CtpHelper.cpp
        helper/DataLoader.cpp
        model/CandleData.cpp
        model/CtpData.cpp
        model/CtpInstrument.cpp
        model/CtpOrder.cpp
        model/TradeSession.cpp
        trade/TradeStatusManager.cpp
        trade/TradeManager.cpp
        )

add_library(ctp_common_lib STATIC ${SRCS})
target_link_libraries(ctp_common_lib midas_common_lib)