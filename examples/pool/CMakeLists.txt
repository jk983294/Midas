SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(poolsrc
        main.cpp
        )

add_executable(pool ${poolsrc})
target_link_libraries(pool ${Boost_LIBRARIES} tbb pthread)
