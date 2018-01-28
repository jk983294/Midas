SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(statssrc
        main.cpp
        InstrumentStats.cpp
        )

add_executable(ctp_stats ${statssrc})
target_link_libraries(ctp_stats midas_common_lib ctp_common_lib)
target_link_libraries(ctp_stats ${Boost_LIBRARIES} tbb pthread)

