cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR) # because of CMAKE_CXX_STANDARD

if(POLICY CMP0075)
  cmake_policy(SET CMP0075 NEW)
endif()

if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

if(POLICY CMP0104)
  cmake_policy(SET CMP0104 NEW)
endif()

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

# by default, force c++17, because openMPT headers require it
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# in case we have any c code, use the best c standard ever
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(${CMAKE_VERSION} VERSION_GREATER "3.6.0")
find_program(CLANG_TIDY
  NAMES "clang-tidy"
  DOC "Path to clang-tidy executable")
  
    if(CLANG_TIDY)
        # whenever clang-tidy is available, use it to automatically add braces after ever "make"
        set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,readability-braces-around-statements;-fix")
#         set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-fix")
    endif(CLANG_TIDY)
endif(${CMAKE_VERSION} VERSION_GREATER "3.6.0")


### set compile flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Warray-bounds -Wstack-protector")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -O0 -Wextra -fstack-check -fstack-protector-all -fno-inline-functions -fsanitize=address") #  -D_GLIBCXX_DEBUG")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if(USE_LIBCXX)
        set(STDCXX "libc++")
    else(USE_LIBCXX)
        set(STDCXX "libstdc++")
    endif(USE_LIBCXX)
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -stdlib=${STDCXX}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -O0 -Wextra -Wdocumentation -fstack-protector-all -fno-inline-functions -fsanitize=address")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g3 -Rpass=loop-vectorize -Rpass-analysis=loop-vectorize")
endif()

add_compile_definitions(_USE_MATH_DEFINES)

### setup project include dirs
set(PROJECT_LOCAL_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/cereal/include
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

find_package ( OpenMP QUIET )
if ( OpenMP_FOUND OR OpenMP_C_FOUND )
    message(STATUS "Found OpenMP ${OpenMP_C_SPEC_DATE}")
    # require at least OMP 3.0
    if ( ( NOT OpenMP_C_SPEC_DATE LESS "200805" ) OR NOT ( OpenMP_C_VERSION VERSION_LESS "3.0" ) )
        set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}" )
        set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}" )
        set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}" )
        set ( LD_FLAGS "${OpenMP_C_LIBRARIES};${LD_FLAGS}" )
    endif()
endif()

###################################
### HANDLE AUDIO OUTPUT SUPPORT ###
###################################
option(ENABLE_ALSA "Compile with support to use ALSA for playback" ON)
option(ENABLE_JACK "Compile with support to use Jack for playback" ON)
option(ENABLE_PORTAUDIO "Compile with support to use PortAudio for playback" ON)
option(ENABLE_EBUR128 "Enable support for noise normalization through libebur128" ON)

if(ENABLE_ALSA)
  find_package(ALSA)
  if(ALSA_FOUND)
        MY_PRINT(ALSA)
        add_definitions(-DUSE_ALSA)
        set(USE_ALSA TRUE)
        
  endif(ALSA_FOUND)
  
  summary_add("ALSA support" USE_ALSA)
endif(ENABLE_ALSA)


if(ENABLE_JACK)
  PKG_CHECK_MODULES(JACK jack IMPORTED_TARGET)
  PKG_CHECK_MODULES(SAMPLERATE samplerate IMPORTED_TARGET)
  
  if(JACK_FOUND AND LIBSAMPLERATE_FOUND)
        add_definitions(-DUSE_JACK)
        set(USE_JACK TRUE)
        
        MY_PRINT(JACK)
  endif(JACK_FOUND AND LIBSAMPLERATE_FOUND)
  
  
  summary_add("Jack support" USE_JACK)
        
endif(ENABLE_JACK)


if(ENABLE_PORTAUDIO)
  PKG_CHECK_MODULES(PORTAUDIO portaudio-2.0 IMPORTED_TARGET)
  
  if(PORTAUDIO_FOUND)
        add_definitions(-DUSE_PORTAUDIO)
        set(USE_PORTAUDIO TRUE)

        MY_PRINT(PORTAUDIO)
  endif(PORTAUDIO_FOUND)
        
    summary_add("PortAudio support" USE_PORTAUDIO)
endif(ENABLE_PORTAUDIO)


if(ENABLE_EBUR128)
  # previous versions neither provided pkgconfig, nor did true peak calc work
  PKG_CHECK_MODULES(EBUR128 libebur128>=1.2.3 IMPORTED_TARGET)
  
  if(EBUR128_FOUND)
        add_definitions(-DUSE_EBUR128)
        set(USE_EBUR128 TRUE)

        MY_PRINT(EBUR128)
  endif(EBUR128_FOUND)
  
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
option(ENABLE_MODPLUG "Use libmodplug to support multiple tracker formats." OFF)
option(ENABLE_OPENMPT "A better libmodplug to support multiple tracker formats." ON)

if(ENABLE_LIBGME)
    MY_FIND_LIB(LIBGME gme)
    
    if(LIBGME_FOUND)
      add_definitions(-DUSE_LIBGME)
      set(USE_LIBGME TRUE)

      MY_PRINT(LIBGME)
    endif(LIBGME_FOUND)
    summary_add("libgme support" LIBGME_FOUND)
endif(ENABLE_LIBGME)


if(ENABLE_LIBMAD)
    PKG_CHECK_MODULES(ID3TAG id3tag IMPORTED_TARGET)
  
  if(ID3TAG_FOUND)
        MY_PRINT(ID3TAG)
        
        MY_FIND_LIB(LIBMAD mad)
        
        if(LIBMAD_FOUND)
            set(USE_MAD TRUE)
        endif(LIBMAD_FOUND)
  
  endif(ID3TAG_FOUND)
  
  summary_add("libmad support" USE_MAD)
endif(ENABLE_LIBMAD)


if(ENABLE_LIBSND)
    PKG_CHECK_MODULES(LIBSND sndfile IMPORTED_TARGET)

    if(LIBSND_FOUND)
      add_definitions(-DUSE_LIBSND)
      set(USE_LIBSND TRUE)

      MY_PRINT(LIBSND)
    endif(LIBSND_FOUND)
    summary_add("libsndfile support" LIBSND_FOUND)
endif(ENABLE_LIBSND)


if(ENABLE_LAZYUSF)
    MY_FIND_LIB(LAZYUSF lazyusf2)
    MY_FIND_LIB(PSFLIB psflib)
    MY_FIND_PKG(ZLIB ZLIB)
    
    # TODO: this is very ugly, any easier way??
    if(${PSFLIB_FOUND})
        if(${ZLIB_FOUND})
            if(${LAZYUSF_FOUND})
                set(USE_LAZYUSF TRUE)
                add_definitions(-DUSE_LAZYUSF)
                if(WIN32)
                    set(LD_FLAGS ${LD_FLAGS} "OLE32" "Shell32")
                endif()
            else()
                message(STATUS "Couldn't find lazyusf")
            endif()
         else()
             message(STATUS "Couldn't find zlib")
        endif()
    else()
        message(STATUS "Couldn't find psflib")
    endif()
    
    summary_add("lazyusf support" USE_LAZYUSF)
endif(ENABLE_LAZYUSF)


if(ENABLE_AOPSF)
    MY_FIND_LIB(AOPSF aopsf)
    MY_FIND_LIB(PSFLIB psflib)
    
    if(${AOPSF_FOUND} AND ${PSFLIB_FOUND})
        set(USE_AOPSF TRUE)
        add_definitions(-DUSE_AOPSF)
    endif(${AOPSF_FOUND} AND ${PSFLIB_FOUND})
    
    summary_add("aopsf support" USE_AOPSF)
endif(ENABLE_AOPSF)


if(ENABLE_VGMSTREAM)
    MY_FIND_LIB(VGMSTREAM vgmstream)
    MY_FIND_LIB(VORBISFILE vorbisfile)
    MY_FIND_LIB(VORBIS vorbis)
    MY_FIND_LIB(MPG123 mpg123)
    MY_FIND_LIB(M m)
    MY_FIND_PKG(ZLIB ZLIB)
  
    if(VGMSTREAM_FOUND AND VORBISFILE_FOUND AND VORBIS_FOUND AND MPG123_FOUND AND ZLIB_FOUND)
      add_definitions(-DUSE_VGMSTREAM)
      set(USE_VGMSTREAM TRUE)

      MY_PRINT(VGMSTREAM)
    endif()
    summary_add("vgmstream support" USE_VGMSTREAM)
endif(ENABLE_VGMSTREAM)


if(ENABLE_FFMPEG)
  PKG_CHECK_MODULES(AVCODEC libavcodec IMPORTED_TARGET)
  PKG_CHECK_MODULES(AVFORMAT libavformat IMPORTED_TARGET)
  PKG_CHECK_MODULES(AVUTIL libavutil IMPORTED_TARGET)
  PKG_CHECK_MODULES(SWRESAMPLE libswresample IMPORTED_TARGET)
  
  if(AVCODEC_FOUND AND AVFORMAT_FOUND AND AVUTIL_FOUND AND SWRESAMPLE_FOUND)
    set(USE_FFMPEG TRUE)
    add_definitions(-DUSE_FFMPEG)
    MY_PRINT(AVCODEC)
    MY_PRINT(AVFORMAT)
    MY_PRINT(AVUTIL)
    MY_PRINT(SWRESAMPLE)
  endif()
  
  summary_add("FFmpeg support" USE_FFMPEG)
endif(ENABLE_FFMPEG)


if(ENABLE_FLUIDSYNTH)
  PKG_CHECK_MODULES(FLUIDSYNTH fluidsynth>=2.2.0 IMPORTED_TARGET)
  PKG_CHECK_MODULES(LIBSMF smf IMPORTED_TARGET)

  if(FLUIDSYNTH_FOUND)
    add_definitions(-DUSE_FLUIDSYNTH)
    set(USE_FLUIDSYNTH TRUE)
    if(DEFINED FLUIDSYNTH_DEFAULT_SF2)
        add_definitions(-DFLUIDSYNTH_DEFAULT_SF2="${FLUIDSYNTH_DEFAULT_SF2}")
    endif()
    MY_PRINT(FLUIDSYNTH)
  
    if(LIBSMF_FOUND)
      add_definitions(-DUSE_SMF)
      set(USE_SMF TRUE)
      MY_PRINT(LIBSMF)
    endif(LIBSMF_FOUND)
  endif(FLUIDSYNTH_FOUND)

  summary_add("fluidsynth support" USE_FLUIDSYNTH)
endif(ENABLE_FLUIDSYNTH)


if(ENABLE_MODPLUG)
  PKG_CHECK_MODULES(LIBMODPLUG libmodplug IMPORTED_TARGET)

  if(LIBMODPLUG_FOUND)
    add_definitions(-DUSE_MODPLUG)

# modplug ships an sndfile.h, which conflicts with libsndfile's sndfile.h
# so dont include modplugs include path in compiler search list to avoid wrong includes
#    set(PROJECT_SYSTEM_INCLUDE_DIRS ${PROJECT_SYSTEM_INCLUDE_DIRS} ${LIBMODPLUG_INCLUDE_DIRS})
    set(LD_FLAGS ${LD_FLAGS} ${LIBMODPLUG_LIBRARIES})
    link_directories(${LIBMODPLUG_LIBRARY_DIRS})

    MY_PRINT(LIBMODPLUG)

    set(USE_MODPLUG TRUE)
  endif(LIBMODPLUG_FOUND)

  summary_add("modplug support" USE_MODPLUG)
endif(ENABLE_MODPLUG)

if(ENABLE_OPENMPT)
  PKG_CHECK_MODULES(LIBOPENMPT libopenmpt IMPORTED_TARGET)

  if(LIBOPENMPT_FOUND)
    add_definitions(-DUSE_OPENMPT)
    set(USE_OPENMPT TRUE)

    MY_PRINT(LIBOPENMPT)
  endif(LIBOPENMPT_FOUND)

  summary_add("openmpt support" USE_OPENMPT)
endif(ENABLE_OPENMPT)

##################################
### miscellaneous dependencies ###
##################################
option(ENABLE_CUE "Enable support for loading cue sheets" ON)
option(ENABLE_GUI "Enable building the QT GUI" ON)
option(ENABLE_VISUALIZER "Enable nice blinky audio visualizers" ON)

if(ENABLE_CUE)
  PKG_CHECK_MODULES(LIBCUE libcue IMPORTED_TARGET)
  
  if(LIBCUE_FOUND)
        add_definitions(-DUSE_CUE)
        set(USE_CUE TRUE)
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

# Additional targets to perform clang-format/clang-tidy
# Get all project files
file(GLOB_RECURSE
     ALL_SOURCE_FILES
     LIST_DIRECTORIES false
     ${CMAKE_SOURCE_DIR}/*.[chi]
     ${CMAKE_SOURCE_DIR}/*.[chi]pp
     ${CMAKE_SOURCE_DIR}/*.[chi]xx
     ${CMAKE_SOURCE_DIR}/*.cc
     ${CMAKE_SOURCE_DIR}/*.hh
     ${CMAKE_SOURCE_DIR}/*.ii
     ${CMAKE_SOURCE_DIR}/*.[CHI]
     )

# Adding clang-format target if executable is found
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
  add_custom_target(
    format
    COMMAND ${CLANG_FORMAT}
    -i
    -style=file
    ${ALL_SOURCE_FILES}
    )
endif(CLANG_FORMAT)

