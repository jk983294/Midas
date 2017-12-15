SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(dbexamplesrc
        main.cpp
        ../../midas_ctp/model/CandleData.cpp
        ../../midas_ctp/dao/CandleDao.cpp
        ../../midas_ctp/dao/InstrumentInfoDao.cpp
        ../../midas_ctp/trade/TradeStatusManager.cpp
        )

add_executable(db_example ${dbexamplesrc})
target_link_libraries(db_example ${Boost_LIBRARIES} mysqlcppconn tbb)
