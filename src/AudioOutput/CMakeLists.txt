project(anmp-audio-output)

include_directories(${PROJECT_INCLUDE_DIR})

SET(ANMP_AUDIO_SRC
       IAudioOutput.cpp
       WaveOutput.cpp
)


if(USE_ALSA)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} ALSAOutput.cpp)
endif(USE_ALSA)

if(USE_JACK)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} JackOutput.cpp)
endif(USE_JACK)

if(USE_PORTAUDIO)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} PortAudioOutput.cpp)
endif(USE_PORTAUDIO)


if(EBUR128_FOUND)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} ebur128Output.cpp)
endif(EBUR128_FOUND)

#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_AUDIO_SRC})
