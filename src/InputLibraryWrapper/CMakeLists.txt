project(anmp-input)

include_directories(${PROJECT_INCLUDE_DIR})


SET(ANMP_INPUT_SRC
       Song.cpp
       StandardWrapper.cpp
       SongFormat.cpp       
)

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
    
    if(LIBGME_FOUND)
        add_definitions(-DUSE_LIBGME)
        set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibGMEWrapper.cpp)
    endif(LIBGME_FOUND)
endif(ENABLE_LIBGME)


if(ENABLE_LIBMAD)
  MY_FIND_LIB(LIBMAD mad)

  if(LIBMAD_FOUND)
        add_definitions(-DUSE_LIBMAD)
        set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibMadWrapper.cpp)
  endif(LIBMAD_FOUND)
endif(ENABLE_LIBMAD)


if(ENABLE_LIBSND)
  MY_FIND_LIB(LIBSND sndfile)

  if(LIBSND_FOUND)
        add_definitions(-DUSE_LIBSND)
        set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibSNDWrapper.cpp)
  endif(LIBSND_FOUND)
endif(ENABLE_LIBSND)


if(ENABLE_LAZYUSF)

    MY_FIND_LIB(LAZYUSF lazyusf2)
    MY_FIND_LIB(PSFLIB psflib)
    MY_FIND_PKG(ZLIB ZLIB)
    
    if(LAZYUSF_FOUND AND PSFLIB_FOUND AND ZLIB_FOUND)
        add_definitions(-DUSE_LAZYUSF)
        set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LazyusfWrapper.cpp)
    endif(LAZYUSF_FOUND AND PSFLIB_FOUND AND ZLIB_FOUND)
endif(ENABLE_LAZYUSF)


if(ENABLE_VGMSTREAM)
  
  MY_FIND_LIB(VGMSTREAM vgmstream)
  MY_FIND_LIB(VORBISFILE vorbisfile)
  MY_FIND_LIB(MPG123 mpg123)
  MY_FIND_LIB(M m)
  MY_FIND_PKG(ZLIB ZLIB)
  MY_FIND_PKG(THREADS Threads)
                         
  if(VGMSTREAM_FOUND)
        add_definitions(-DUSE_VGMSTREAM)
        set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} VGMStreamWrapper.cpp)
  endif(VGMSTREAM_FOUND)
endif(ENABLE_VGMSTREAM)


if(ENABLE_FFMPEG)
  MY_FIND_PKG(FFMPEG FFmpeg)
  
  if(FFMPEG_FOUND)
    add_definitions(-DUSE_FFMPEG)
    SET(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} FFMpegWrapper.cpp)
  endif(FFMPEG_FOUND)
endif(ENABLE_FFMPEG)


#---------------------------------------------------------------------------------------------

ADD_LIBRARY(${LIB_ANMP_INPUT} ${ANMP_INPUT_SRC})
target_link_libraries(${LIB_ANMP_INPUT} ${LD_FLAGS})

# set(LD_FLAGS ${LD_FLAGS} ${LIB_ANMP_INPUT})
