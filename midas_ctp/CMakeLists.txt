SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../")

set(MidasCtpSrc
        main.cpp
        core/CtpProcess.cpp
        core/TradeSpi.cpp
        core/MdSpi.cpp
        core/CtpAdmin.cpp
        core/CtpConfig.cpp
        core/TradeManager.cpp
        )

add_executable(midas ${MidasCtpSrc})
target_link_libraries(midas ${Boost_LIBRARIES} tbb)
target_link_libraries(midas pthread)
target_link_libraries(midas thostmduserapi thosttraderapi)
