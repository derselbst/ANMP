project(anmp-input)


SET(ANMP_INPUT_SRC
       Song.cpp
       StandardWrapper.cpp   
)

if(LIBGME_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibGMEWrapper.cpp)
endif(LIBGME_FOUND)


if(USE_LIBMAD)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibMadWrapper.cpp)
endif(USE_LIBMAD)


if(LIBSND_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibSNDWrapper.cpp)
endif(LIBSND_FOUND)


if(LAZYUSF_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LazyusfWrapper.cpp)
endif(LAZYUSF_FOUND)


if(VGMSTREAM_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} VGMStreamWrapper.cpp)
endif(VGMSTREAM_FOUND)

if(USE_FFMPEG)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} FFMpegWrapper.cpp)
endif(USE_FFMPEG)

if(USE_FLUIDSYNTH)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} FluidsynthWrapper.cpp)
endif(USE_FLUIDSYNTH)



#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_INPUT_SRC})
