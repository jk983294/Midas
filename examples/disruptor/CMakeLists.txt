SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(disruptorsrc
        main.cpp
        )

add_executable(disruptor ${disruptorsrc})
target_link_libraries(disruptor ${Boost_LIBRARIES} tbb)
target_link_libraries(disruptor pthread)
