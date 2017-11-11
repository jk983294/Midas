SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(statssrc
        main.cpp
        InstrumentStats.cpp
        )

add_executable(ctp_stats ${statssrc})
target_link_libraries(ctp_stats ${Boost_LIBRARIES})
target_link_libraries(ctp_stats pthread)

