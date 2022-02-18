project(anmp-input)


SET(ANMP_INPUT_SRC
       Song.cpp
       Song.h
       SongInfo.h
       StandardWrapper.cpp
       StandardWrapper.h
       StandardWrapper_impl.h
)

if(LIBGME_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibGMEWrapper.cpp LibGMEWrapper.h)
endif(LIBGME_FOUND)


if(USE_MAD)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibMadWrapper.cpp LibMadWrapper.h)
endif(USE_MAD)


if(LIBSND_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LibSNDWrapper.cpp LibSNDWrapper.h)
endif(LIBSND_FOUND)


if(USE_LAZYUSF)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} LazyusfWrapper.cpp LazyusfWrapper.h)
endif(USE_LAZYUSF)

if(USE_AOPSF)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} AopsfWrapper.cpp AopsfWrapper.h)
endif(USE_AOPSF)

if(VGMSTREAM_FOUND)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} VGMStreamWrapper.cpp VGMStreamWrapper.h)
endif(VGMSTREAM_FOUND)

if(USE_FFMPEG)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} FFMpegWrapper.cpp FFMpegWrapper.h)
endif(USE_FFMPEG)

if(USE_SMF)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} MidiWrapper.cpp MidiWrapper.h)
endif(USE_SMF)

if(USE_FLUIDSYNTH)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} FluidsynthWrapper.cpp FluidsynthWrapper.h)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} N64CSeqWrapper.cpp N64CSeqWrapper.h)
endif(USE_FLUIDSYNTH)

if(USE_MODPLUG)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} ModPlugWrapper.cpp ModPlugWrapper.h)
endif(USE_MODPLUG)

if(USE_OPENMPT)
    set(ANMP_INPUT_SRC ${ANMP_INPUT_SRC} OpenMPTWrapper.cpp OpenMPTWrapper.h)
endif(USE_OPENMPT)


#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_INPUT_SRC})
