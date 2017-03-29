project(channel-mixer)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(channel_SRCS
    channelconfig.cpp
)

qt5_wrap_ui(UI_HEADERS
    channelconfig.ui
)

ADD_LIBRARY(${LIBMIX} STATIC ${channel_SRCS})
