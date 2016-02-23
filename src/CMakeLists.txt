cmake_minimum_required(VERSION 2.8)

project(ANMP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

include_directories(Common)

SET(libSRCS
AudioOutput/ALSAOutput.cpp
AudioOutput/IAudioOutput.cpp

Common/common.cpp
Common/CustomExceptions.cpp
Common/PlaylistFactory.cpp

InputLibraryWrapper/LazyusfWrapper.cpp
InputLibraryWrapper/LibSNDWrapper.cpp
InputLibraryWrapper/OpusWrapper.cpp
InputLibraryWrapper/SongFormat.cpp
InputLibraryWrapper/VGMStreamWrapper.cpp

PlayerLogic/Player.cpp
PlayerLogic/Playlist.cpp
PlayerLogic/Song.cpp
)
# SET(myprogSRCS test.cpp)

ADD_LIBRARY(libanmp ${libSRCS})
