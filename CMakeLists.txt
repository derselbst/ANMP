cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR) # because of CMAKE_CXX_STANDARD
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


# Global setting: build everything position independent
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# by default, force c++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


### set compile flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic-errors -Wall -Warray-bounds -Wstack-protector")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -Wextra -fstack-check -fstack-protector-all -D_GLIBCXX_DEBUG")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -Wextra -Wdocumentation -fstack-protector-all")
endif()


### setup project include dirs
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/Common ${CMAKE_SOURCE_DIR}/src/PlayerLogic ${CMAKE_SOURCE_DIR}/src/InputLibraryWrapper ${CMAKE_SOURCE_DIR}/src/AudioOutput)


### Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(utils)
include(FindPkgConfig)
include(summary)
include(GNUInstallDirs)

#############################
### SCAN FOR DEPENDENCIES ###
#############################
find_package (Threads REQUIRED)
set(LD_FLAGS ${LD_FLAGS} ${CMAKE_THREAD_LIBS_INIT})

###################################
### HANDLE AUDIO OUTPUT SUPPORT ###
###################################
option(ENABLE_ALSA "Compile with support to use ALSA for playback" ON)
option(ENABLE_JACK "Compile with support to use Jack for playback" ON)
option(ENABLE_PORTAUDIO "Compile with support to use PortAudio for playback" ON)
option(ENABLE_EBUR128 "Enable support for noise normalization through libebur128" ON)

if(ENABLE_ALSA)
  MY_FIND_PKG(ALSA ALSA)
  
  summary_add("ALSA support" ALSA_FOUND)
endif(ENABLE_ALSA)


if(ENABLE_JACK)
  PKG_CHECK_MODULES(JACK jack)
  PKG_CHECK_MODULES(SRC samplerate)
  
  if(JACK_FOUND AND SRC_FOUND)
        add_definitions(-DUSE_JACK)
        set(PROJECT_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} ${JACK_INCLUDE_DIRS})
        set(PROJECT_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} ${SRC_INCLUDE_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${JACK_LIBRARIES})
        set(LD_FLAGS ${LD_FLAGS} ${SRC_LIBRARIES})
        
        MY_PRINT(JACK)
        MY_PRINT(SRC)
        
        summary_add("Jack support" TRUE)
  else(JACK_FOUND AND SRC_FOUND)
        summary_add("Jack support" FALSE)
  endif(JACK_FOUND AND SRC_FOUND)
        
endif(ENABLE_JACK)


if(ENABLE_PORTAUDIO)
  PKG_CHECK_MODULES(PORTAUDIO portaudio-2.0)
  
  if(PORTAUDIO_FOUND)
        add_definitions(-DUSE_PORTAUDIO)
        set(PROJECT_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} ${PORTAUDIO_INCLUDE_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${PORTAUDIO_LIBRARIES})
        
        MY_PRINT(PORTAUDIO)
  endif(PORTAUDIO_FOUND)
        
    summary_add("PortAudio support" PORTAUDIO_FOUND)
endif(ENABLE_PORTAUDIO)


if(ENABLE_EBUR128)
  MY_FIND_LIB(EBUR128 ebur128)
  
  summary_add("ebur128 support" EBUR128_FOUND)
endif(ENABLE_EBUR128)

############################
### HANDLE CODEC SUPPORT ###
############################
option(ENABLE_LIBGME "Use blargg's game-music-emu to support several video game emulated music formats" ON)
option(ENABLE_LIBMAD "Use MAD to support MPEG audio formats" ON)
option(ENABLE_LIBSND "Use sndfile to support common audio formats (FLAC, WAVE, OGG, etc.)" ON)
option(ENABLE_LAZYUSF "Use lazyusf to support USF (Ultra64 Sound Format)" ON)
option(ENABLE_VGMSTREAM "Use vgmstream to support innumerable streamed audio formats from video games." ON)
option(ENABLE_FFMPEG "Use ffmpeg to support innumerable streamed audio formats, also from video streams." ON)

if(ENABLE_LIBGME)
    MY_FIND_LIB(LIBGME gme)
    
    summary_add("libgme support" LIBGME_FOUND)
endif(ENABLE_LIBGME)


if(ENABLE_LIBMAD)
    PKG_CHECK_MODULES(ID3TAG id3tag)
  
  if(ID3TAG_FOUND)
        
        set(PROJECT_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} ${ID3TAG_INCLUDE_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${ID3TAG_LIBRARIES})
        MY_PRINT(ID3TAG)
        
        MY_FIND_LIB(LIBMAD mad)
        
  
        summary_add("libmad support" LIBMAD_FOUND)
  endif(ID3TAG_FOUND)
endif(ENABLE_LIBMAD)


if(ENABLE_LIBSND)
  MY_FIND_LIB(LIBSND sndfile)
  
    summary_add("libsndfile support" LIBSND_FOUND)
endif(ENABLE_LIBSND)


if(ENABLE_LAZYUSF)
    MY_FIND_LIB(LAZYUSF lazyusf)
    MY_FIND_LIB(PSFLIB psflib)
    MY_FIND_PKG(ZLIB ZLIB)
    
    # TODO: this is very ugly, any easier way??
    if(${LAZYUSF_FOUND})
        if(${PSFLIB_FOUND})
            if(${ZLIB_FOUND})
                set(LAZYUSF_FOUND TRUE)
            endif(${ZLIB_FOUND})
        endif(${PSFLIB_FOUND})
    endif(${LAZYUSF_FOUND})
    
    summary_add("lazyusf support" LAZYUSF_FOUND)
endif(ENABLE_LAZYUSF)


if(ENABLE_VGMSTREAM)
  MY_FIND_LIB(VGMSTREAM vgmstream)
  MY_FIND_LIB(VORBISFILE vorbisfile)
  MY_FIND_LIB(MPG123 mpg123)
  MY_FIND_LIB(M m)
  MY_FIND_PKG(ZLIB ZLIB)
  
  summary_add("vgmstream support" VGMSTREAM_FOUND)
endif(ENABLE_VGMSTREAM)


if(ENABLE_FFMPEG)
  MY_FIND_PKG(FFMPEG FFmpeg)
  
  summary_add("FFmpeg support" FFMPEG_FOUND)
endif(ENABLE_FFMPEG)


if(ENABLE_FLUIDSYNTH)
  PKG_CHECK_MODULES(FLUIDSYNTH fluidsynth)
  
  if(FLUIDSYNTH_FOUND)
        add_definitions(-DUSE_FLUIDSYNTH)
        set(PROJECT_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} ${FLUIDSYNTH_INCLUDE_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${FLUIDSYNTH_LIBRARIES})
        
        MY_PRINT(FLUIDSYNTH)
  endif(FLUIDSYNTH_FOUND)
        
    summary_add("FLUIDSYNTH support" FLUIDSYNTH_FOUND)
endif(ENABLE_FLUIDSYNTH)


##################################
### miscellaneous dependencies ###
##################################
option(ENABLE_CUE "Enable support for loading cue sheets" ON)
option(ENABLE_GUI "Enable building the QT GUI" ON)

if(ENABLE_CUE)
  PKG_CHECK_MODULES(LIBCUE libcue)
  
  if(LIBCUE_FOUND)
        add_definitions(-DUSE_CUE)
        set(PROJECT_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} ${LIBCUE_INCLUDE_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${LIBCUE_LIBRARIES})
        
        MY_PRINT(LIBCUE)
  endif(LIBCUE_FOUND)
        
    summary_add("libcue support" LIBCUE_FOUND)
endif(ENABLE_CUE)


if(ENABLE_GUI)
    # Qt library
    MY_FIND_PKG(Qt5Core Qt5Core)
    
    MY_FIND_PKG(Qt5OpenGL Qt5OpenGL)

    # OpenGL & GLEW library
    MY_FIND_PKG(OPENGL OpenGL)
    
    # Executables fail to build with Qt 5 in the default configuration
    # without -fPIE. We add that here.
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
    
    # We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
#     add_definitions(${Qt5Widgets_DEFINITIONS})

    # TODO: this is very ugly, any easier way??
    if(${Qt5Widgets_FOUND})
        if(${OPENGL_FOUND})
            if(${Qt5OpenGL_FOUND})
                set(QT5_FOUND TRUE)
            endif(${Qt5OpenGL_FOUND})
        endif(${OPENGL_FOUND})
    endif(${Qt5Widgets_FOUND})
    
    summary_add("QT GUI support" QT5_FOUND)
endif(ENABLE_GUI)


message("")
### Show a summary of what we got
summary_show()


include_directories(${PROJECT_INCLUDE_DIR})

# anmp-core
add_subdirectory(src)

# anmp-qt
add_subdirectory(gui)

# anmp-normalize
add_subdirectory(utils)

