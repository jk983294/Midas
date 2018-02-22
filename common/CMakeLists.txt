file(GLOB CommonSrcs "*/*.cpp" "*/*/*.cpp")
add_library(midas_common_lib STATIC ${CommonSrcs} midas/md/MdLock.cpp midas/md/MdLock.h)