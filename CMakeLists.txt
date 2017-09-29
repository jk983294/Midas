cmake_minimum_required(VERSION 3.5)


project(midas)

LINK_DIRECTORIES(/home/myproject/libs)

# boost lib
find_package( Boost 1.58.0 COMPONENTS system thread program_options )
include_directories(${Boost_INCLUDE_DIRS})

# tbb lib
list(APPEND CMAKE_MODULE_PATH "/home/$USER/github/CommonScript/linux/dev/compile/FindTBB/")
find_package(TBB COMPONENTS tbbmalloc tbbmalloc_proxy)

# use tbb lock
add_definitions(-DMIDAS_LOCK_TYPE_TBB)
# add_definitions(-DMY_KEY=MY_VALUE)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -Wall ")
message ("cxx Flags: " ${CMAKE_CXX_FLAGS})

# Source code
include_directories(common)
include_directories(lib)
include_directories("/home/$USER/include")

add_subdirectory(test)
add_subdirectory(examples)
add_subdirectory(midas_ctp)
