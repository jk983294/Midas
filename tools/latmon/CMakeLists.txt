SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(latmonsrc main.cpp)

add_executable(latmon ${latmonsrc})
target_link_libraries(latmon midas_common_lib)
target_link_libraries(latmon ${Boost_LIBRARIES})

set(latcmpsrc latcmp.cpp)
add_executable(latcmp ${latcmpsrc})
target_link_libraries(latcmp midas_common_lib)
target_link_libraries(latcmp ${Boost_LIBRARIES})

