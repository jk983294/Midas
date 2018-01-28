SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(disruptor_object_src
        main.cpp
        )

add_executable(disruptor_object ${disruptor_object_src})
target_link_libraries(disruptor_object midas_common_lib)
target_link_libraries(disruptor_object ${Boost_LIBRARIES} tbb pthread)
