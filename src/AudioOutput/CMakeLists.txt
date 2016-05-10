project(anmp-audio-output)

include_directories(${PROJECT_INCLUDE_DIR})

SET(ANMP_AUDIO_SRC
       IAudioOutput.cpp
       WaveOutput.cpp
)


if(ALSA_FOUND)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} ALSAOutput.cpp)
endif(ALSA_FOUND)


if(EBUR128_FOUND)
    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} ebur128Output.cpp)
endif(EBUR128_FOUND)

#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_AUDIO_SRC})
