cmake_minimum_required(VERSION 2.8)

project(anmp-core)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Warray-bounds")

INCLUDE_DIRECTORIES(SYSTEM Common PlayerLogic InputLibraryWrapper AudioOutput)

SET(LIBANMP_SRC
       AudioOutput/IAudioOutput.cpp
       Common/AtomicWrite.cpp
       Common/Common.cpp
       Common/PlaylistFactory.cpp
       InputLibraryWrapper/Song.cpp
       InputLibraryWrapper/StandardWrapper.cpp
       InputLibraryWrapper/SongFormat.cpp       
       PlayerLogic/Config.cpp
       PlayerLogic/Player.cpp
       PlayerLogic/Playlist.cpp
)

############################
### HANDLE CODEC SUPPORT ###
############################
option(USE_LIBGME "Use libgme to support several video game emulated music formats" ON)
option(USE_LIBMAD "Use libmad to support MPEG audio formats" ON)
option(USE_LIBSND "Use libsndfile to support common audio formats (FLAC, WAVE, OGG, etc.)" ON)
option(USE_LAZYUSF "Use lazyusf2 to support USF (Ultra64 Sound Format)" ON)
option(USE_VGMSTREAM "Use vgmstream to support innumerable streamed audio formats from video games." ON)
option(USE_CUE "Use libcue to support loading cue sheets" ON)

if(USE_LIBGME)
  set (LIBGME_INSTALL_DIR "" CACHE FILEPATH "Directory where libgme is installed")
  
  FIND_LIBRARY(LIBGME_LIBRARIES gme ${LIBGME_INSTALL_DIR})
  
  if(LIBGME_LIBRARIES)
	set (LIBGME_INCLUDE_DIR "/usr/include/gme" CACHE FILEPATH "Directory where header files for libgme are installed")
	INCLUDE_DIRECTORIES(${LIBGME_INCLUDE_DIR})
	
        add_definitions(-DUSE_LIBGME)
        set(LIBANMP_SRC ${LIBANMP_SRC} InputLibraryWrapper/LibGMEWrapper.cpp)
        
        set(LD_FLAGS ${LD_FLAGS} ${LIBGME_LIBRARIES})
        
        message("\tlibgme include dir  : ${LIBGME_INCLUDE_DIR}")
        message("\tlibgme libraries    : ${LIBGME_LIBRARIES}")
  else(LIBGME_LIBRARIES)
        message(SEND_ERROR "libgme not found. Define LIBGME_INSTALL_DIR or turn USE_LIBGME off.")
  endif(LIBGME_LIBRARIES)
endif(USE_LIBGME)


if(USE_LIBMAD)
  set (LIBMAD_INSTALL_DIR "" CACHE FILEPATH "Directory where libmad is installed")
  
  FIND_LIBRARY(LIBMAD_LIBRARIES mad ${LIBMAD_INSTALL_DIR})
  
  if(LIBMAD_LIBRARIES)
	set (LIBMADFILE_INCLUDE_DIR "/usr/include" CACHE FILEPATH "Directory where header files for libmad are installed")
	INCLUDE_DIRECTORIES(${LIBMADFILE_INCLUDE_DIR})
	
        add_definitions(-DUSE_LIBMAD)
        set(LIBANMP_SRC ${LIBANMP_SRC} InputLibraryWrapper/LibMadWrapper.cpp)
        
        set(LD_FLAGS ${LD_FLAGS} ${LIBMAD_LIBRARIES})
        
        message("\tlibmad include dir  : ${LIBMADFILE_INCLUDE_DIR}")
        message("\tlibmad libraries    : ${LIBMAD_LIBRARIES}")
  else(LIBMAD_LIBRARIES)
        message(SEND_ERROR "libmad not found. Define LIBMAD_INSTALL_DIR or turn USE_LIBMAD off.")
  endif(LIBMAD_LIBRARIES)
endif(USE_LIBMAD)


if(USE_LIBSND)
  set (LIBSND_INSTALL_DIR "" CACHE FILEPATH "Directory where libsndfile is installed")
  
  FIND_LIBRARY(LIBSND_LIBRARIES sndfile ${LIBSND_INSTALL_DIR})
  
  if(LIBSND_LIBRARIES)
	set (LIBSNDFILE_INCLUDE_DIR "/usr/include" CACHE FILEPATH "Directory where header files for libsndfile are installed")
	INCLUDE_DIRECTORIES(${LIBSNDFILE_INCLUDE_DIR})
	
        add_definitions(-DUSE_LIBSND)
        set(LIBANMP_SRC ${LIBANMP_SRC} InputLibraryWrapper/LibSNDWrapper.cpp)
        
        set(LD_FLAGS ${LD_FLAGS} ${LIBSND_LIBRARIES})
        
        message("\tLibSND include dir  : ${LIBSNDFILE_INCLUDE_DIR}")
        message("\tLibSND libraries    : ${LIBSND_LIBRARIES}")
  else(LIBSND_LIBRARIES)
        message(SEND_ERROR "libsndfile not found. Define LIBSND_INSTALL_DIR or turn USE_LIBSND off.")
  endif(LIBSND_LIBRARIES)
endif(USE_LIBSND)


if(USE_LAZYUSF)
  set (LAZYUSF_INSTALL_DIR "" CACHE FILEPATH "Directory where lazyusf2 is installed")
  set (PSFLIB_INSTALL_DIR "" CACHE FILEPATH "Directory where psflib is installed")
  
  FIND_LIBRARY(LAZYUSF_LIBRARIES lazyusf2 ${LAZYUSF_INSTALL_DIR})
  FIND_LIBRARY(PSFLIB_LIBRARIES psflib ${PSFLIB_INSTALL_DIR})
  find_package(ZLIB REQUIRED)
  
  if(LAZYUSF_LIBRARIES AND PSFLIB_LIBRARIES)
	set (LAZYUSF_INCLUDE_DIR "/usr/include/lazyusf2" CACHE FILEPATH "Directory where header files for lazyusf2 are installed")
	set (PSFLIB_INCLUDE_DIR "/usr/include/psflib" CACHE FILEPATH "Directory where header files for psflib are installed")
	INCLUDE_DIRECTORIES(${LAZYUSF_INCLUDE_DIR} ${PSFLIB_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
	
        add_definitions(-DUSE_LAZYUSF)
        set(LIBANMP_SRC ${LIBANMP_SRC} InputLibraryWrapper/LazyusfWrapper.cpp)
        
        set(LD_FLAGS ${LD_FLAGS} ${LAZYUSF_LIBRARIES} ${PSFLIB_LIBRARIES} ${ZLIB_LIBRARIES})
        
        message("\tlazyusf2 include dir: ${LAZYUSF_INCLUDE_DIR}")
        message("\tlazyusf2 libraries  : ${LAZYUSF_LIBRARIES}")
        message("\tpsflib   include dir: ${PSFLIB_INCLUDE_DIR}")
        message("\tpsflib   libraries  : ${PSFLIB_LIBRARIES}")
        message("\tzlib     include dir: ${ZLIB_INCLUDE_DIRS}")
        message("\tzlib     libraries  : ${ZLIB_LIBRARIES}")
  else(LAZYUSF_LIBRARIES AND PSFLIB_LIBRARIES)
        if(NOT LAZYUSF_LIBRARIES)
            message(SEND_ERROR "lazyusf2 not found. Define LAZYUSF_INSTALL_DIR or turn USE_LAZYUSF off.")
        endif(NOT LAZYUSF_LIBRARIES)
        if(NOT PSFLIB_LIBRARIES)
            message(SEND_ERROR "psflib not found. lazyusf2 requires a library for reading PSF files. Define PSFLIB_INSTALL_DIR or turn USE_LAZYUSF off.")
        endif(NOT PSFLIB_LIBRARIES)
  endif(LAZYUSF_LIBRARIES AND PSFLIB_LIBRARIES)
endif(USE_LAZYUSF)


if(USE_VGMSTREAM)
  set (VGMSTREAM_INSTALL_DIR "" CACHE FILEPATH "Directory where vgmstream is installed")
  
  FIND_LIBRARY(VGMSTREAM_LIBRARIES vgmstream ${VGMSTREAM_INSTALL_DIR})
  FIND_LIBRARY(VORBISFILE_LIBRARIES vorbisfile)
  FIND_LIBRARY(MPG123_LIBRARIES mpg123)
  find_library(M_LIB m)
  find_package(ZLIB REQUIRED)
  find_package(Threads REQUIRED)
  
  if(VGMSTREAM_LIBRARIES)
	set (VGMSTREAM_INCLUDE_DIR "/usr/include/vgmstream" CACHE FILEPATH "Directory where header files for vgmstream are installed")
	INCLUDE_DIRECTORIES(${VGMSTREAM_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
	
        add_definitions(-DUSE_VGMSTREAM)
        set(LIBANMP_SRC ${LIBANMP_SRC} InputLibraryWrapper/VGMStreamWrapper.cpp)
        
        set(LD_FLAGS ${LD_FLAGS} ${VGMSTREAM_LIBRARIES} ${ZLIB_LIBRARIES} ${MPG123_LIBRARIES} ${VORBISFILE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${M_LIB})
        
        message("\tvgmstream  include dir: ${VGMSTREAM_INCLUDE_DIR}")
        message("\tvgmstream  libraries  : ${VGMSTREAM_LIBRARIES}")
        message("\tzlib       include dir: ${ZLIB_INCLUDE_DIRS}")
        message("\tzlib       libraries  : ${ZLIB_LIBRARIES}")
        message("\tvorbisfile libraries  : ${VORBISFILE_LIBRARIES}")
        message("\tmpg123     libraries  : ${MPG123_LIBRARIES}")
  else(VGMSTREAM_LIBRARIES)
            message(SEND_ERROR "vgmstream not found. Define VGMSTREAM_INSTALL_DIR or turn USE_VGMSTREAM off.")
  endif(VGMSTREAM_LIBRARIES)
endif(USE_VGMSTREAM)


if(USE_CUE)
  set (LIBCUE_INSTALL_DIR "" CACHE FILEPATH "Directory where libsndfile is installed")
  
  FIND_LIBRARY(LIBCUE_LIBRARIES cue ${LIBCUE_INSTALL_DIR})
  
  if(LIBCUE_LIBRARIES)
	set (LIBCUE_INCLUDE_DIR "/usr/include/libcue-1.4" CACHE FILEPATH "Directory where header files for libcue are installed")
	INCLUDE_DIRECTORIES(${LIBCUE_INCLUDE_DIR})
	
        add_definitions(-DUSE_CUE)
        
        set(LD_FLAGS ${LD_FLAGS} ${LIBCUE_LIBRARIES})
        
        message("\tlibcue   include dir: ${LIBCUE_INCLUDE_DIR}")
        message("\tlibcue   libraries  : ${LIBCUE_LIBRARIES}")
  else(LIBCUE_LIBRARIES)
        message(SEND_ERROR "libcue not found. Define LIBCUE_INSTALL_DIR or turn USE_CUE off.")
  endif(LIBCUE_LIBRARIES)
endif(USE_CUE)

#---------------------------------------------------------------------------------------------

###################################
### HANDLE AUDIO OUTPUT SUPPORT ###
###################################
option(USE_ALSA "Compile with support to use ALSA for playback" ON)


if(USE_ALSA)
  find_package(ALSA REQUIRED)
  
  add_definitions(-DUSE_ALSA)
  SET(LIBANMP_SRC ${LIBANMP_SRC} AudioOutput/ALSAOutput.cpp)
  
  set(LD_FLAGS ${LD_FLAGS} ${ALSA_LIBRARIES})
  
  message("\tALSA  include dir : ${ALSA_INCLUDE_DIR}")
  message("\tALSA  libraries   : ${ALSA_LIBRARIES}")
endif(USE_ALSA)

#---------------------------------------------------------------------------------------------



### TODO: check that there is at least one codec and one audio lib support activated

# define name of the anmp core library
SET(LIBANMP anmpcore)
ADD_LIBRARY(${LIBANMP} ${LIBANMP_SRC})

set(LD_FLAGS ${LD_FLAGS} ${LIBANMP})

add_subdirectory(GUI)


# add_executable(main test.cpp)
# target_link_libraries(main ${LIBANMP} ${LD_FLAGS})

