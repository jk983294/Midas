SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(statssrc
        main.cpp
        InstrumentStats.cpp
        ../../midas_ctp/trade/TradeStatusManager.cpp
        ../../midas_ctp/model/CtpInstrument.cpp
        ../../midas_ctp/model/CtpData.cpp
        ../../midas_ctp/model/CandleData.cpp
        )

add_executable(ctp_stats ${statssrc})
target_link_libraries(ctp_stats ${Boost_LIBRARIES} tbb)
target_link_libraries(ctp_stats pthread)

