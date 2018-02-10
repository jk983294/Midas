SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(expsrc main.cpp)
add_executable(exp ${expsrc})
target_link_libraries(exp midas_common_lib ctp_common_lib)
target_link_libraries(exp ${Boost_LIBRARIES} mysqlcppconn tbb pthread rt)