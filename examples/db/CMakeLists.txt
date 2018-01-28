SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(dbexamplesrc
        main.cpp
        )

add_executable(db_example ${dbexamplesrc})
target_link_libraries(db_example midas_common_lib ctp_common_lib)
target_link_libraries(db_example ${Boost_LIBRARIES} mysqlcppconn tbb)