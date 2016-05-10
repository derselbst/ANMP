cmake_minimum_required(VERSION 2.8)
project (ANMP)

### setup binary output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)


### Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "")
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  message(STATUS "")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


### set compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -fstack-protector-all -Wall -Warray-bounds")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -Wextra -Weffc++")


### setup project include dirs
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/Common ${CMAKE_SOURCE_DIR}/src/PlayerLogic ${CMAKE_SOURCE_DIR}/src/InputLibraryWrapper ${CMAKE_SOURCE_DIR}/src/AudioOutput)


### Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(utils)
include(FindPkgConfig)

#############################
### SCAN FOR DEPENDENCIES ###
#############################

###################################
### HANDLE AUDIO OUTPUT SUPPORT ###
###################################
option(ENABLE_ALSA "Compile with support to use ALSA for playback" ON)
option(ENABLE_EBUR128 "support for noise normalization through libebur128" ON)

if(ENABLE_ALSA)
  MY_FIND_PKG(ALSA ALSA)
endif(ENABLE_ALSA)

if(ENABLE_EBUR128)
  MY_FIND_LIB(EBUR128 ebur128)
endif(ENABLE_EBUR128)

############################
### HANDLE CODEC SUPPORT ###
############################
option(ENABLE_LIBGME "Use libgme to support several video game emulated music formats" ON)
option(ENABLE_LIBMAD "Use libmad to support MPEG audio formats" ON)
option(ENABLE_LIBSND "Use libsndfile to support common audio formats (FLAC, WAVE, OGG, etc.)" ON)
option(ENABLE_LAZYUSF "Use lazyusf2 to support USF (Ultra64 Sound Format)" ON)
option(ENABLE_VGMSTREAM "Use vgmstream to support innumerable streamed audio formats from video games." ON)
option(ENABLE_FFMPEG "Use ffmpeg to support innumerable streamed audio formats, also from video streams." ON)

if(ENABLE_LIBGME)
    MY_FIND_LIB(LIBGME gme)
endif(ENABLE_LIBGME)


if(ENABLE_LIBMAD)
  MY_FIND_LIB(LIBMAD mad)
endif(ENABLE_LIBMAD)


if(ENABLE_LIBSND)
  MY_FIND_LIB(LIBSND sndfile)
endif(ENABLE_LIBSND)


if(ENABLE_LAZYUSF)
    MY_FIND_LIB(LAZYUSF lazyusf2)
    MY_FIND_LIB(PSFLIB psflib)
    MY_FIND_PKG(ZLIB ZLIB)
endif(ENABLE_LAZYUSF)


if(ENABLE_VGMSTREAM)
  MY_FIND_LIB(VGMSTREAM vgmstream)
  MY_FIND_LIB(VORBISFILE vorbisfile)
  MY_FIND_LIB(MPG123 mpg123)
  MY_FIND_LIB(M m)
  MY_FIND_PKG(ZLIB ZLIB)
  MY_FIND_PKG(THREADS Threads)
endif(ENABLE_VGMSTREAM)


if(ENABLE_FFMPEG)
  MY_FIND_PKG(FFMPEG FFmpeg)
endif(ENABLE_FFMPEG)


##################################
### miscellaneous dependencies ###
##################################
option(ENABLE_CUE "Use libcue to support loading cue sheets" ON)

if(ENABLE_CUE)
  PKG_CHECK_MODULES(LIBCUE libcue)
  
  if(LIBCUE_FOUND)
        add_definitions(-DUSE_CUE)
        INCLUDE_DIRECTORIES(${LIBCUE_INCLUDE_DIRS})
#         link_directories(${LIBCUE_LIBRARY_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${LIBCUE_LIBRARIES})
        
        MY_PRINT(LIBCUE)
  endif(LIBCUE_FOUND)
endif(ENABLE_CUE)




add_subdirectory(src)
