SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

file(GLOB MidasCtpBacktestSrc "*.cpp")
add_executable(backtest ${MidasCtpBacktestSrc})
target_link_libraries(backtest midas_common_lib ctp_common_lib)
target_link_libraries(backtest ${Boost_LIBRARIES} tbb pthread z mysqlcppconn)
