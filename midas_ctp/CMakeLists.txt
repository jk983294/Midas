SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../")

set(MidasCtpSrc
        main.cpp
        core/CtpProcess.cpp
        core/TradeSpi.cpp
        core/MdSpi.cpp
        core/CtpAdmin.cpp
        core/CtpConfig.cpp
        core/TradeManager.cpp
        core/CtpDataConsumer.cpp
        model/CtpData.cpp
        model/CandleData.cpp
        model/CtpInstrument.cpp
        trade/TradeStatusManager.cpp
        )

add_executable(midas ${MidasCtpSrc})
target_link_libraries(midas ${Boost_LIBRARIES} tbb)
target_link_libraries(midas pthread)
target_link_libraries(midas thostmduserapi thosttraderapi)
target_link_libraries(midas z)

set(MidasCtpBacktestSrc
        backtest/main.cpp
        backtest/CtpBackTester.cpp
        backtest/CtpBackTesterAdmin.cpp
        backtest/CtpBackTesterConfig.cpp
        model/CandleData.cpp
        )

add_executable(backtest ${MidasCtpBacktestSrc})
target_link_libraries(backtest ${Boost_LIBRARIES} tbb)
target_link_libraries(backtest pthread)
target_link_libraries(midas thostmduserapi thosttraderapi)
target_link_libraries(midas z)
