project(anmp-audio-output)

SET(ANMP_AUDIO_SRC
       IAudioOutput.cpp
       IAudioOutput.h
       IAudioOutput_impl.h
       WaveOutput.cpp
       WaveOutput.h
)


if(USE_ALSA)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} ALSAOutput.cpp ALSAOutput.h)
endif(USE_ALSA)

if(USE_JACK)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} JackOutput.cpp JackOutput.h)
endif(USE_JACK)

if(USE_PORTAUDIO)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} PortAudioOutput.cpp PortAudioOutput.h)
endif(USE_PORTAUDIO)


if(EBUR128_FOUND)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} ebur128Output.cpp ebur128Output.h)
endif(EBUR128_FOUND)

#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_AUDIO_SRC})
