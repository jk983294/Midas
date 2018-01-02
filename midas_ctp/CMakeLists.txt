SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../")

set(MidasCommonSrc
        model/CtpData.cpp
        model/CandleData.cpp
        model/CtpInstrument.cpp
        model/TradeSession.cpp
        trade/TradeStatusManager.cpp
        dao/CandleDao.cpp
        dao/InstrumentInfoDao.cpp
        )

set(MidasCtpSrc
        main.cpp
        core/CtpProcess.cpp
        core/TradeSpi.cpp
        core/MdSpi.cpp
        core/CtpAdmin.cpp
        core/CtpConfig.cpp
        trade/TradeManager.cpp
        core/CtpDataConsumer.cpp
        )

add_executable(midas ${MidasCtpSrc} ${MidasCommonSrc})
target_link_libraries(midas ${Boost_LIBRARIES} tbb pthread z mysqlcppconn)
target_link_libraries(midas thostmduserapi thosttraderapi)

set(MidasCtpBacktestSrc
        backtest/main.cpp
        backtest/CtpBackTester.cpp
        backtest/CtpBackTesterAdmin.cpp
        backtest/CtpBackTesterConfig.cpp
        helper/DataLoader.cpp
        )

add_executable(backtest ${MidasCtpBacktestSrc} ${MidasCommonSrc})
target_link_libraries(backtest ${Boost_LIBRARIES} tbb pthread z mysqlcppconn)
