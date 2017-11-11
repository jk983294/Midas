SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(dpsrc
        main.cpp
        )

add_executable(delta_play ${dpsrc})
target_link_libraries(delta_play ${Boost_LIBRARIES})
target_link_libraries(delta_play pthread)
target_link_libraries(delta_play z)

