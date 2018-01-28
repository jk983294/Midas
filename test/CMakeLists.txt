SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../")

set(tests
        main.cpp
        utils/TestLog.cpp
        utils/TestConvertHelper.cpp
        utils/TestTimeHelper.cpp
        utils/TestRegExpHelper.cpp
        midas/TestMidasConfig.cpp
        midas/TestMidasTick.cpp
        net/TestBuffer.cpp
        net/TestChannel.cpp
        net/TestIpAddress.cpp
        net/TestNetworkHelper.cpp
        math/TestMathHelper.cpp
        time/TestTimestamp.cpp
#        net/TestTcp.cpp
#        net/TestUdp.cpp
        )

enable_testing()

add_executable(test.all ${tests})
target_link_libraries(test.all midas_common_lib ctp_common_lib)
target_link_libraries(test.all ${Boost_LIBRARIES} tbb pthread)
