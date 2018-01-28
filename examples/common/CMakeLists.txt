SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(EchoClientSrc EchoClient.cpp)
add_executable(EchoClient ${EchoClientSrc})
target_link_libraries(EchoClient midas_common_lib)
target_link_libraries(EchoClient ${Boost_LIBRARIES} tbb pthread)

set(EchoServerSrc EchoServer.cpp)
add_executable(EchoServer ${EchoServerSrc})
target_link_libraries(EchoServer midas_common_lib)
target_link_libraries(EchoServer ${Boost_LIBRARIES} tbb pthread)

set(ioSrc io.cpp)
add_executable(io ${ioSrc})
target_link_libraries(io midas_common_lib)
target_link_libraries(io ${Boost_LIBRARIES} tbb pthread)