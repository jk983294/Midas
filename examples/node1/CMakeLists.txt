SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(node1src
        main.cpp
        Node1Process.cpp
        Node1ProcessConfig.cpp
        )

add_executable(node1 ${node1src})
target_link_libraries(node1 midas_common_lib)
target_link_libraries(node1 ${Boost_LIBRARIES} tbb pthread)
