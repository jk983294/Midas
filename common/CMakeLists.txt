set(SRCS
        utils/RegExpHelper.cpp
        utils/convert/TimeHelper.cpp
        )

add_library(midas_common_lib STATIC ${SRCS})