SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

file(GLOB JnlSrc "*.cpp")

add_executable(delta_jnl ${JnlSrc})
target_link_libraries(delta_jnl midas_common_lib)
target_link_libraries(delta_jnl ${Boost_LIBRARIES} pthread z)

