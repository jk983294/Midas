SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(node3src
        main.cpp
        Node3Process.cpp
        )

add_executable(node3 ${node3src})
target_link_libraries(node3 midas_common_lib)
target_link_libraries(node3 ${Boost_LIBRARIES} tbb pthread)
