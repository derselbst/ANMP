cmake_minimum_required(VERSION 2.8)

project(ANMP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

include_directories(Common PlayerLogic InputLibraryWrapper AudioOutput)

find_package(ALSA REQUIRED)

set (LIBSND_INSTALL_DIR "" CACHE FILEPATH "Directory where libsndfile was installed")
##find libraries installed in the system
FIND_LIBRARY(LIBSNDFILE sndfile ${LIBSND_INSTALL_DIR})

##set particular settings
# if( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
#   set(os_link_libs /usr/lib)
#   set(os_libs ${LIBSNDFILE})
# endif()

# INCLUDE_DIRECTORIES(SYSTEM ${os_include_libs} ${LIBSND_INSTALL_DIR}/include)

message("LibSND libraries: ${LIBSNDFILE}")


set(LD_FLAGS ${LD_FLAGS} ${ALSA_LIBRARIES} ${LIBSNDFILE})
message("ALSA include dir: ${ALSA_INCLUDE_DIR}")
message("ALSA libraries: ${ALSA_LIBRARIES}")

SET(libSRCS
AudioOutput/ALSAOutput.cpp
AudioOutput/IAudioOutput.cpp

Common/Common.cpp
Common/PlaylistFactory.cpp

# InputLibraryWrapper/LazyusfWrapper.cpp
InputLibraryWrapper/LibSNDWrapper.cpp
# InputLibraryWrapper/OpusWrapper.cpp
InputLibraryWrapper/SongFormat.cpp
# InputLibraryWrapper/VGMStreamWrapper.cpp

PlayerLogic/Config.cpp
PlayerLogic/Player.cpp
PlayerLogic/Playlist.cpp
PlayerLogic/Song.cpp
)


ADD_LIBRARY(anmp ${libSRCS})

SET(testProgSRC test.cpp)


add_executable(main ${testProgSRC})
target_link_libraries(main anmp ${LD_FLAGS})

