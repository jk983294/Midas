SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(node2src
        main.cpp
        Node2Process.cpp
        )

add_executable(node2 ${node2src})
target_link_libraries(node2 ${Boost_LIBRARIES} tbb pthread)
