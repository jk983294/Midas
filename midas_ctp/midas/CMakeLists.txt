SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(MidasCtpSrc
        main.cpp
        CtpProcess.cpp
        TradeSpi.cpp
        MdSpi.cpp
        CtpAdmin.cpp
        CtpConfig.cpp
        CtpDataConsumer.cpp
        )

add_executable(midas ${MidasCtpSrc})
target_link_libraries(midas midas_common_lib ctp_common_lib)
target_link_libraries(midas ${Boost_LIBRARIES} tbb pthread z mysqlcppconn thostmduserapi thosttraderapi)
