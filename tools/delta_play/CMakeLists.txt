SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(dpsrc
        main.cpp
        )

add_executable(delta_play ${dpsrc})
target_link_libraries(delta_play midas_common_lib)
target_link_libraries(delta_play ${Boost_LIBRARIES} pthread z)

