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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fstack-protector-all -Wall -Warray-bounds")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -Wextra -Weffc++")


### setup project include dirs
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/Common ${CMAKE_SOURCE_DIR}/src/PlayerLogic ${CMAKE_SOURCE_DIR}/src/InputLibraryWrapper ${CMAKE_SOURCE_DIR}/src/AudioOutput)


### Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(utils)
include(FindPkgConfig)


### setup library names we will build in the subdirs
# core library made up with following sub-libs:
SET(LIB_ANMP_AUDIO_OUTPUT anmp-audio-output)
SET(LIB_ANMP_COMMON       anmp-common)
SET(LIB_ANMP_INPUT        anmp-input)
SET(LIB_ANMP_PLAYER       anmp-player)

SET(LIB_ANMP_CORE ${LIB_ANMP_AUDIO_OUTPUT} ${LIB_ANMP_COMMON} ${LIB_ANMP_INPUT} ${LIB_ANMP_PLAYER})


add_subdirectory(src)
