SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(jnlsrc
        main.cpp
        )

add_executable(Journal ${jnlsrc})
target_link_libraries(Journal midas_common_lib)
target_link_libraries(Journal ${Boost_LIBRARIES} tbb pthread z)

