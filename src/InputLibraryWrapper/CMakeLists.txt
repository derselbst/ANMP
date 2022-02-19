project(anmp-input)


SET(ANMP_INPUT_SRC
       Song.cpp
       Song.h
       SongInfo.h
       StandardWrapper.cpp
       StandardWrapper.h
       StandardWrapper_impl.h
)

add_library(${PROJECT_NAME} OBJECT ${ANMP_INPUT_SRC})

if(LIBGME_FOUND)
    target_sources(${PROJECT_NAME} PRIVATE LibGMEWrapper.cpp LibGMEWrapper.h)
endif(LIBGME_FOUND)


if(USE_MAD)
    target_sources(${PROJECT_NAME} PRIVATE LibMadWrapper.cpp LibMadWrapper.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::ID3TAG)
endif(USE_MAD)


if(LIBSND_FOUND)
    target_sources(${PROJECT_NAME} PRIVATE LibSNDWrapper.cpp LibSNDWrapper.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::LIBSND)
endif(LIBSND_FOUND)


if(USE_LAZYUSF)
    target_sources(${PROJECT_NAME} PRIVATE LazyusfWrapper.cpp LazyusfWrapper.h)
endif(USE_LAZYUSF)

if(USE_AOPSF)
    target_sources(${PROJECT_NAME} PRIVATE AopsfWrapper.cpp AopsfWrapper.h)
endif(USE_AOPSF)

if(VGMSTREAM_FOUND)
    target_sources(${PROJECT_NAME} PRIVATE VGMStreamWrapper.cpp VGMStreamWrapper.h)
endif(VGMSTREAM_FOUND)

if(USE_FFMPEG)
    target_sources(${PROJECT_NAME} PRIVATE FFMpegWrapper.cpp FFMpegWrapper.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::AVCODEC PkgConfig::AVFORMAT PkgConfig::AVUTIL PkgConfig::SWRESAMPLE)
endif(USE_FFMPEG)

if(USE_SMF)
    target_sources(${PROJECT_NAME} PRIVATE MidiWrapper.cpp MidiWrapper.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::LIBSMF)
endif(USE_SMF)

if(USE_FLUIDSYNTH)
    target_sources(${PROJECT_NAME} PRIVATE FluidsynthWrapper.cpp FluidsynthWrapper.h)
    target_sources(${PROJECT_NAME} PRIVATE N64CSeqWrapper.cpp N64CSeqWrapper.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::FLUIDSYNTH)
endif(USE_FLUIDSYNTH)

if(USE_MODPLUG)
    target_sources(${PROJECT_NAME} PRIVATE ModPlugWrapper.cpp ModPlugWrapper.h)
endif(USE_MODPLUG)

if(USE_OPENMPT)
    target_sources(${PROJECT_NAME} PRIVATE OpenMPTWrapper.cpp OpenMPTWrapper.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::LIBOPENMPT)
endif(USE_OPENMPT)
