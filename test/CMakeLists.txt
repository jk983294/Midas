SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../")

file(GLOB tests "*.cpp" "*/*.cpp" "*/*/*.cpp")

enable_testing()

add_executable(test.all ${tests})
target_link_libraries(test.all midas_common_lib ctp_common_lib)
target_link_libraries(test.all ${Boost_LIBRARIES} tbb pthread)
