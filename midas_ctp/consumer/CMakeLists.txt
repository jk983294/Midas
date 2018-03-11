SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

file(GLOB ConsumerCtpSrc "*.cpp")
add_executable(consumer ${ConsumerCtpSrc} )
target_link_libraries(consumer midas_common_lib ctp_common_lib)
target_link_libraries(consumer ${Boost_LIBRARIES} tbb pthread z rt thostmduserapi thosttraderapi)
