cmake_minimum_required(VERSION 2.8)

project(anmp-core)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Warray-bounds -Wc++11-compat")

INCLUDE_DIRECTORIES(SYSTEM Common PlayerLogic InputLibraryWrapper AudioOutput)


SET(LIBANMP_SRC

       AudioOutput/IAudioOutput.cpp
       Common/Common.cpp
       Common/PlaylistFactory.cpp
       InputLibraryWrapper/Song.cpp
       InputLibraryWrapper/SongFormat.cpp
       PlayerLogic/Config.cpp
       PlayerLogic/Player.cpp
       PlayerLogic/Playlist.cpp
)


### HANDLE CODEC SUPPORT ###
option(USE_LIBSND "Use libsnd to support common audio formats (FLAC, WAVE, OGG, etc.)" ON)
option(USE_LAZYUSF "Use lazyusf2 to support USF (Ultra64 Sound Format)" ON)
option(USE_CUE "Use libcue to support loading cue sheets" ON)

if(USE_LIBSND)
  set (LIBSND_INSTALL_DIR "" CACHE FILEPATH "Directory where libsndfile is installed")
  
  FIND_LIBRARY(LIBSNDFILE sndfile ${LIBSND_INSTALL_DIR})
  
  if(LIBSNDFILE)
        add_definitions(-DUSE_LIBSND)
        set(LIBANMP_SRC ${LIBANMP_SRC} InputLibraryWrapper/LibSNDWrapper.cpp)
        
        set(LD_FLAGS ${LD_FLAGS} ${LIBSNDFILE})
        
        message("LibSND libraries: ${LIBSNDFILE}")
  else(LIBSNDFILE)
        message(FATAL_ERROR "libsndfile not found. Define LIBSND_INSTALL_DIR or turn USE_LIBSND off.")
  endif(LIBSNDFILE)
endif(USE_LIBSND)

if(USE_LAZYUSF)
  set (LAZYUSF_INSTALL_DIR "" CACHE FILEPATH "Directory where lazyusf2 is installed")
  set (PSFLIB_INSTALL_DIR "" CACHE FILEPATH "Directory where psflib is installed")
  
  FIND_LIBRARY(LIBLAZYUSF lazyusf2 ${LAZYUSF_INSTALL_DIR})
  FIND_LIBRARY(LIBPSFLIB psflib ${PSFLIB_INSTALL_DIR})
  
  if(LIBLAZYUSF AND LIBPSFLIB)
        add_definitions(-DUSE_LAZYUSF)
        set(LIBANMP_SRC ${LIBANMP_SRC} InputLibraryWrapper/LazyusfWrapper.cpp)
        
        set(LD_FLAGS ${LD_FLAGS} ${LIBLAZYUSF} ${LIBPSFLIB})
        
        message("lazyusf2 libraries: ${LIBLAZYUSF}")
        message("psflib   libraries: ${LIBPSFLIB}")
  else(LIBLAZYUSF AND LIBPSFLIB)
        if(NOT LIBLAZYUSF)
            message(SEND_ERROR "lazyusf2 not found. Define LAZYUSF_INSTALL_DIR or turn USE_LAZYUSF off.")
        endif(NOT LIBLAZYUSF)
        if(NOT LIBPSFLIB)
            message(SEND_ERROR "psflib not found. lazyusf2 requires a library for reading PSF files. Define PSFLIB_INSTALL_DIR or turn USE_LAZYUSF off.")
        endif(NOT LIBPSFLIB)
  endif(LIBLAZYUSF AND LIBPSFLIB)
endif(USE_LAZYUSF)


if(USE_CUE)
  set (LIBCUE_INSTALL_DIR "" CACHE FILEPATH "Directory where libsndfile is installed")
  
  FIND_LIBRARY(LIBCUE cue ${LIBCUE_INSTALL_DIR})
  
  if(LIBCUE)
        add_definitions(-DUSE_CUE)
        
        set(LD_FLAGS ${LD_FLAGS} ${LIBCUE})
        
        message("libcue libraries: ${LIBCUE}")
  else(LIBCUE)
        message(FATAL_ERROR "libcue not found. Define LIBCUE_INSTALL_DIR or turn USE_CUE off.")
  endif(LIBCUE)
endif(USE_CUE)



### HANDLE AUDIO OUTPUT SUPPORT ###
option(USE_ALSA "Compile with support to use ALSA for playback" ON)


if(USE_ALSA)
  find_package(ALSA REQUIRED)
  
  add_definitions(-DUSE_ALSA)
  SET(LIBANMP_SRC ${LIBANMP_SRC} AudioOutput/ALSAOutput.cpp)
  
  set(LD_FLAGS ${LD_FLAGS} ${ALSA_LIBRARIES})
  
  message("ALSA include dir: ${ALSA_INCLUDE_DIR}")
  message("ALSA libraries: ${ALSA_LIBRARIES}")
endif(USE_ALSA)

###################################


### TODO: check that there is at least one codec and one audio lib support activated

# define name of the anmp core library
SET(LIBANMP anmp)
ADD_LIBRARY(${LIBANMP} ${LIBANMP_SRC})




add_executable(main test.cpp)
target_link_libraries(main ${LIBANMP} ${LD_FLAGS})

