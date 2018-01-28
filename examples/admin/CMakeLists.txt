SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(admin1src
        AdminMain.cpp
        )

add_executable(admin ${admin1src})
target_link_libraries(admin midas_common_lib)
target_link_libraries(admin ${Boost_LIBRARIES} tbb pthread)
