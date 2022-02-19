project(anmp-audio-output)

SET(ANMP_AUDIO_SRC
       IAudioOutput.cpp
       IAudioOutput.h
       IAudioOutput_impl.h
       WaveOutput.cpp
       WaveOutput.h
)

add_library(${PROJECT_NAME} OBJECT ${ANMP_AUDIO_SRC})

if(USE_ALSA)
    target_sources(${PROJECT_NAME} PRIVATE ALSAOutput.cpp ALSAOutput.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC ALSA::ALSA)
endif(USE_ALSA)

if(USE_JACK)
    target_sources(${PROJECT_NAME} PRIVATE JackOutput.cpp JackOutput.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::JACK PkgConfig::SAMPLERATE)
endif(USE_JACK)

if(USE_PORTAUDIO)
    target_sources(${PROJECT_NAME} PRIVATE PortAudioOutput.cpp PortAudioOutput.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::PORTAUDIO)
endif(USE_PORTAUDIO)


if(EBUR128_FOUND)
    target_sources(${PROJECT_NAME} PRIVATE ebur128Output.cpp ebur128Output.h)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::EBUR128)
endif(EBUR128_FOUND)

#---------------------------------------------------------------------------------------------

