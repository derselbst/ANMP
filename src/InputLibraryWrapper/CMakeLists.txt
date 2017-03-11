project(anmp-input)


SET(ANMP_INPUT_SRC
       Song.cpp
       StandardWrapper.cpp   
)

if(LIBGME_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibGMEWrapper.cpp)
endif(LIBGME_FOUND)


if(USE_MAD)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibMadWrapper.cpp)
endif(USE_MAD)


if(LIBSND_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibSNDWrapper.cpp)
endif(LIBSND_FOUND)


if(USE_LAZYUSF)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LazyusfWrapper.cpp)
endif(USE_LAZYUSF)

if(USE_AOPSF)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} AopsfWrapper.cpp)
endif(USE_AOPSF)

if(VGMSTREAM_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} VGMStreamWrapper.cpp)
endif(VGMSTREAM_FOUND)

if(USE_FFMPEG)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} FFMpegWrapper.cpp)
endif(USE_FFMPEG)

if(USE_SMF)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} MidiWrapper.cpp)
endif(USE_SMF)

if(USE_FLUIDSYNTH)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} FluidsynthWrapper.cpp)
endif(USE_FLUIDSYNTH)

if(USE_MODPLUG)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} ModPlugWrapper.cpp)
endif(USE_MODPLUG)


#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_INPUT_SRC})
