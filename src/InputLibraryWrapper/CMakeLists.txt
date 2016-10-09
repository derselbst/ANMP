project(anmp-input)

include_directories(${PROJECT_INCLUDE_DIR})


SET(ANMP_INPUT_SRC
       Song.cpp
       StandardWrapper.cpp   
)

if(LIBGME_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibGMEWrapper.cpp)
endif(LIBGME_FOUND)


if(LIBMAD_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibMadWrapper.cpp)
endif(LIBMAD_FOUND)


if(LIBSND_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibSNDWrapper.cpp)
endif(LIBSND_FOUND)


if(LAZYUSF_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LazyusfWrapper.cpp)
endif(LAZYUSF_FOUND)


if(VGMSTREAM_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} VGMStreamWrapper.cpp)
endif(VGMSTREAM_FOUND)

if(FFMPEG_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} FFMpegWrapper.cpp)
endif(FFMPEG_FOUND)


#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_INPUT_SRC})
