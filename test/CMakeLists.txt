SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../")

set(tests
        main.cpp
        utils/TestLog.cpp
        utils/TestConvertHelper.cpp
        utils/TestTimeHelper.cpp
        midas/TestMidasConfig.cpp
        midas/TestMidasTick.cpp
        net/TestBuffer.cpp
        net/TestChannel.cpp
        net/TestIpAddress.cpp
        net/TestNetworkHelper.cpp
        math/TestMathHelper.cpp
#        net/TestTcp.cpp
#        net/TestUdp.cpp
        )

enable_testing()

add_executable(test.all ${tests})
target_link_libraries(test.all ${Boost_LIBRARIES} tbb)
target_link_libraries(test.all pthread)
