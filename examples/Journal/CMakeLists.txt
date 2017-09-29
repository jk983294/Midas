SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

set(jnlsrc
        main.cpp
        )

add_executable(Journal ${jnlsrc})
target_link_libraries(Journal ${Boost_LIBRARIES} tbb)
target_link_libraries(Journal pthread)
target_link_libraries(Journal z) # zip lib

