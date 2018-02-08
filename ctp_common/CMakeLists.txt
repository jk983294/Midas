file(GLOB SOURCES "*/*.cpp")

add_library(ctp_common_lib STATIC ${SOURCES})
target_link_libraries(ctp_common_lib midas_common_lib)