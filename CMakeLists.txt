cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR) # because of CMAKE_CXX_STANDARD
project (ANMP)

### setup binary output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)


### Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "")
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  message(STATUS "")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


# Global setting: build everything position independent
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# by default, force c++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# in case we have any c code, use the best c standard ever
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)


### set compile flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Warray-bounds -Wstack-protector")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -Wextra -fstack-check -fstack-protector-all -fno-inline-functions -D_GLIBCXX_DEBUG")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if(USE_LIBCXX)
        set(STDCXX "libc++")
    else(USE_LIBCXX)
        set(STDCXX "libstdc++")
    endif(USE_LIBCXX)
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -stdlib=${STDCXX}")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -Wextra -Wdocumentation -fstack-protector-all -fno-inline-functions -fsanitize=bounds,float-cast-overflow,float-divide-by-zero,object-size,integer,vptr  -stdlib=${STDCXX}")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g3 -O2 -Rpass=loop-vectorize -Rpass-analysis=loop-vectorize  -stdlib=${STDCXX}")
endif()


### setup project include dirs
set(PROJECT_LOCAL_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/Common
        ${CMAKE_SOURCE_DIR}/src/PlayerLogic
        ${CMAKE_SOURCE_DIR}/src/InputLibraryWrapper
        ${CMAKE_SOURCE_DIR}/src/AudioOutput)


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
  find_package(ALSA)
  HANDLE_FOUND(ALSA)
  
  if(ALSA_FOUND)
        add_definitions(-DUSE_ALSA)
        set(USE_ALSA TRUE)
        
  endif(ALSA_FOUND)
  
  summary_add("ALSA support" USE_ALSA)
endif(ENABLE_ALSA)


if(ENABLE_JACK)
  find_package(Jack)
  HANDLE_FOUND(JACK)
  
  find_package(LibSamplerate)
  HANDLE_FOUND(LIBSAMPLERATE)
  
  if(JACK_FOUND AND LIBSAMPLERATE_FOUND)
        add_definitions(-DUSE_JACK)
        set(USE_JACK TRUE)
        
  endif(JACK_FOUND AND LIBSAMPLERATE_FOUND)
  
  
  summary_add("Jack support" USE_JACK)
        
endif(ENABLE_JACK)


if(ENABLE_PORTAUDIO)
  PKG_CHECK_MODULES(PORTAUDIO portaudio-2.0)
  
  if(PORTAUDIO_FOUND)
        add_definitions(-DUSE_PORTAUDIO)
        set(USE_PORTAUDIO TRUE)
        set(PROJECT_SYSTEM_INCLUDE_DIRS ${PROJECT_SYSTEM_INCLUDE_DIRS} ${PORTAUDIO_INCLUDE_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${PORTAUDIO_LIBRARIES})
        
        MY_PRINT(PORTAUDIO)
  endif(PORTAUDIO_FOUND)
        
    summary_add("PortAudio support" USE_PORTAUDIO)
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
option(ENABLE_AOPSF "Use aopsf to support PSF1 and PSF2 files (Portable Sound Format)" ON)
option(ENABLE_VGMSTREAM "Use vgmstream to support innumerable streamed audio formats from video games." ON)
option(ENABLE_FFMPEG "Use ffmpeg to support innumerable streamed audio formats, also from video streams." ON)
option(ENABLE_FLUIDSYNTH "Use fluidsynth to synthesize MIDI files." ON)
option(ENABLE_MODPLUG "Use libmodplug to support multiple tracker formats." ON)

if(ENABLE_LIBGME)
    MY_FIND_LIB(LIBGME gme)
    
    summary_add("libgme support" LIBGME_FOUND)
endif(ENABLE_LIBGME)


if(ENABLE_LIBMAD)
    PKG_CHECK_MODULES(ID3TAG id3tag)
  
  if(ID3TAG_FOUND)
        
        set(PROJECT_SYSTEM_INCLUDE_DIRS ${PROJECT_SYSTEM_INCLUDE_DIRS} ${ID3TAG_INCLUDE_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${ID3TAG_LIBRARIES})
        MY_PRINT(ID3TAG)
        
        MY_FIND_LIB(LIBMAD mad)
        
        if(LIBMAD_FOUND)
            set(USE_MAD TRUE)
        endif(LIBMAD_FOUND)
  
  endif(ID3TAG_FOUND)
  
  summary_add("libmad support" USE_MAD)
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
                set(USE_LAZYUSF TRUE)
            endif(${ZLIB_FOUND})
        endif(${PSFLIB_FOUND})
    endif(${LAZYUSF_FOUND})
    
    summary_add("lazyusf support" USE_LAZYUSF)
endif(ENABLE_LAZYUSF)


if(ENABLE_AOPSF)
    MY_FIND_LIB(AOPSF aopsf)
    MY_FIND_LIB(PSFLIB psflib)
    
    if(${AOPSF_FOUND} AND ${PSFLIB_FOUND})
        set(USE_AOPSF TRUE)
    endif(${AOPSF_FOUND} AND ${PSFLIB_FOUND})
    
    summary_add("aopsf support" USE_AOPSF)
endif(ENABLE_AOPSF)


if(ENABLE_VGMSTREAM)
  MY_FIND_LIB(VGMSTREAM vgmstream)
  MY_FIND_LIB(VORBISFILE vorbisfile)
  MY_FIND_LIB(MPG123 mpg123)
  MY_FIND_LIB(M m)
  MY_FIND_PKG(ZLIB ZLIB)
  
  summary_add("vgmstream support" VGMSTREAM_FOUND)
endif(ENABLE_VGMSTREAM)


if(ENABLE_FFMPEG)
  find_package(FFmpeg)
  HANDLE_FOUND(FFMPEG)
  
  if(FFMPEG_FOUND)
    set(USE_FFMPEG TRUE)
    add_definitions(-DUSE_FFMPEG)
  endif(FFMPEG_FOUND)
  
  summary_add("FFmpeg support" USE_FFMPEG)
endif(ENABLE_FFMPEG)


if(ENABLE_FLUIDSYNTH)
  PKG_CHECK_MODULES(FLUIDSYNTH fluidsynth)
  PKG_CHECK_MODULES(LIBSMF smf)
  
  if(LIBSMF_FOUND AND FLUIDSYNTH_FOUND)
                add_definitions(-DUSE_FLUIDSYNTH)
                add_definitions(-DUSE_SMF)
                if(DEFINED FLUIDSYNTH_DEFAULT_SF2)
                    add_definitions(-DFLUIDSYNTH_DEFAULT_SF2="${FLUIDSYNTH_DEFAULT_SF2}")
                endif()
                
                set(PROJECT_SYSTEM_INCLUDE_DIRS ${PROJECT_SYSTEM_INCLUDE_DIRS} ${FLUIDSYNTH_INCLUDE_DIRS})
                set(LD_FLAGS ${LD_FLAGS} ${FLUIDSYNTH_LIBRARIES})
                
                MY_PRINT(FLUIDSYNTH)
                
                
                set(PROJECT_SYSTEM_INCLUDE_DIRS ${PROJECT_SYSTEM_INCLUDE_DIRS} ${LIBSMF_INCLUDE_DIRS})
                set(LD_FLAGS ${LD_FLAGS} ${LIBSMF_LIBRARIES})
                
                MY_PRINT(LIBSMF)
                
                set(USE_FLUIDSYNTH TRUE)
                set(USE_SMF TRUE)
  endif(LIBSMF_FOUND AND FLUIDSYNTH_FOUND)
  
  summary_add("fluidsynth support" USE_FLUIDSYNTH)
endif(ENABLE_FLUIDSYNTH)


if(ENABLE_MODPLUG)
  PKG_CHECK_MODULES(LIBMODPLUG libmodplug)
  
  if(LIBMODPLUG_FOUND)
                add_definitions(-DUSE_MODPLUG)
                
# modplug ships an sndfile.h, which conflicts with libsndfile's sndfile.h
# so dont include modplugs include path in compiler search list to avoid wrong includes
#                 set(PROJECT_SYSTEM_INCLUDE_DIRS ${PROJECT_SYSTEM_INCLUDE_DIRS} ${LIBMODPLUG_INCLUDE_DIRS})
                set(LD_FLAGS ${LD_FLAGS} ${LIBMODPLUG_LIBRARIES})
                
                MY_PRINT(LIBMODPLUG)
                
                set(USE_MODPLUG TRUE)
  endif(LIBMODPLUG_FOUND)
  
  summary_add("modplug support" USE_MODPLUG)
endif(ENABLE_MODPLUG)

##################################
### miscellaneous dependencies ###
##################################
option(ENABLE_CUE "Enable support for loading cue sheets" ON)
option(ENABLE_GUI "Enable building the QT GUI" ON)
option(ENABLE_VISUALIZER "Enable nice blinky audio visualizers" ON)

if(ENABLE_CUE)
  PKG_CHECK_MODULES(LIBCUE libcue)
  
  if(LIBCUE_FOUND)
        add_definitions(-DUSE_CUE)
        set(PROJECT_SYSTEM_INCLUDE_DIRS ${PROJECT_SYSTEM_INCLUDE_DIRS} ${LIBCUE_INCLUDE_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${LIBCUE_LIBRARIES})
        
        MY_PRINT(LIBCUE)
  endif(LIBCUE_FOUND)
        
    summary_add("libcue support" LIBCUE_FOUND)
endif(ENABLE_CUE)


if(ENABLE_GUI)

    find_package(Qt5Widgets)
    HANDLE_FOUND(Qt5Widgets)
    
    find_package(Qt5DBus)
    HANDLE_FOUND(Qt5DBus)
   
    if(${Qt5Widgets_FOUND} AND ${Qt5DBus_FOUND})
        set(USE_GUI TRUE)
        add_definitions(-DUSE_GUI)
        
        # We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
        add_definitions(${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS})
        
        MY_PRINT(QT5CORE)

    endif(${Qt5Widgets_FOUND} AND ${Qt5DBus_FOUND})
    
    summary_add("QT GUI support" USE_GUI)
endif(ENABLE_GUI)


if(ENABLE_VISUALIZER)

  if(NOT USE_GUI)
     message(STATUS "cannot use visualizers when not building gui")
     
  else(NOT USE_GUI)
  
    find_package(Qt5OpenGL)
    HANDLE_FOUND(Qt5OpenGL)

    # OpenGL & GLEW library
    find_package(OpenGL)
    HANDLE_FOUND(OPENGL)
    
    if(Qt5OpenGL_FOUND AND OPENGL_FOUND)
        set(USE_VISUALIZER TRUE)
        add_definitions(-DUSE_VISUALIZER)
    endif(Qt5OpenGL_FOUND AND OPENGL_FOUND)
    
  endif(NOT USE_GUI)
  
  
    summary_add("Audio Visualizer support" USE_VISUALIZER)
  
endif(ENABLE_VISUALIZER)


message("")
### Show a summary of what we got
summary_show()

include_directories(${PROJECT_LOCAL_INCLUDE_DIRS})
include_directories(SYSTEM ${PROJECT_SYSTEM_INCLUDE_DIRS})

# anmp-core
add_subdirectory(src)

# anmp-qt
add_subdirectory(gui)

# anmp-normalize
add_subdirectory(utils)

ENABLE_TESTING()
add_subdirectory(test)

