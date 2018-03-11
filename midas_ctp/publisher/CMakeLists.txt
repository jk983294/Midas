SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

file(GLOB PublisherCtpSrc "*.cpp")
add_executable(publisher ${PublisherCtpSrc})
target_link_libraries(publisher midas_common_lib ctp_common_lib)
target_link_libraries(publisher ${Boost_LIBRARIES} tbb pthread z rt thostmduserapi thosttraderapi)
