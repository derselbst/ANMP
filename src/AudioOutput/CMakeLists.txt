project(anmp-audio-output)

include_directories(${PROJECT_INCLUDE_DIR})

SET(ANMP_AUDIO_SRC
       IAudioOutput.cpp
       WaveOutput.cpp
)


###################################
### HANDLE AUDIO OUTPUT SUPPORT ###
###################################
option(ENABLE_ALSA "Compile with support to use ALSA for playback" ON)
option(ENABLE_EBUR128 "support for noise normalization through libebur128" ON)

if(ENABLE_ALSA)
  MY_FIND_PKG(ALSA ALSA)
  
  if(ALSA_FOUND)
        add_definitions(-DUSE_ALSA)
        SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} ALSAOutput.cpp)
  endif(ALSA_FOUND)
endif(ENABLE_ALSA)


# already did a MY_FIND_LIB call in superordinate dir
if(EBUR128_FOUND)
    add_definitions(-DUSE_EBUR128)

    SET(ANMP_AUDIO_SRC ${ANMP_AUDIO_SRC} ebur128Output.cpp)
endif(EBUR128_FOUND)

#---------------------------------------------------------------------------------------------


ADD_LIBRARY(${LIB_ANMP_AUDIO_OUTPUT} ${ANMP_AUDIO_SRC})
target_link_libraries(${LIB_ANMP_AUDIO_OUTPUT} ${LD_FLAGS})

# set(LD_FLAGS ${LD_FLAGS} ${LIB_ANMP_AUDIO_OUTPUT})

