SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(latmonsrc
        main.cpp
        )

add_executable(latmon ${latmonsrc})
target_link_libraries(latmon midas_common_lib)
target_link_libraries(latmon ${Boost_LIBRARIES})

