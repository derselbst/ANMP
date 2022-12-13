project(anmp)

SET(ANMP_AUDIO_SRC
       AudioOutput/IAudioOutput.cpp
       AudioOutput/IAudioOutput.h
       AudioOutput/IAudioOutput_impl.h
       AudioOutput/WaveOutput.cpp
       AudioOutput/WaveOutput.h
)

SET(ANMP_COMMON_SRC
       Common/AtomicWrite.cpp
       Common/AtomicWrite.h
       Common/AudioDriver.c
       Common/AudioDriver.h
       Common/Common.cpp
       Common/Common.h
       Common/CommonExceptions.h
       Common/Event.h
       Common/LoudnessFile.cpp
       Common/LoudnessFile.h
       Common/Nullable.h
       Common/PlaylistFactory.cpp
       Common/PlaylistFactory.h
       Common/SongFormat.cpp
       Common/SongFormat.h
       Common/StringFormatter.cpp
       Common/StringFormatter.h
       Common/SampleFormat.c
       Common/SampleFormat.h
       Common/ThreadPriority.cpp
       Common/ThreadPriority.h
       Common/tree.h
       Common/types.h
)

SET(ANMP_INPUT_SRC
       InputLibraryWrapper/Song.cpp
       InputLibraryWrapper/Song.h
       InputLibraryWrapper/SongInfo.h
       InputLibraryWrapper/StandardWrapper.cpp
       InputLibraryWrapper/StandardWrapper.h
       InputLibraryWrapper/StandardWrapper_impl.h
)

SET(ANMP_PLAYER_SRC
       PlayerLogic/Config.cpp
       PlayerLogic/Config.h
       PlayerLogic/IPlaylist.h
       PlayerLogic/Player.cpp
       PlayerLogic/Player.h
       PlayerLogic/Playlist.cpp
       PlayerLogic/Playlist.h
)

add_library(anmp-internal OBJECT ${ANMP_AUDIO_SRC} ${ANMP_COMMON_SRC} ${ANMP_INPUT_SRC} ${ANMP_PLAYER_SRC})
if(USE_ALSA)
    target_sources(anmp-internal PRIVATE AudioOutput/ALSAOutput.cpp AudioOutput/ALSAOutput.h)
    target_link_libraries(anmp-internal PUBLIC ALSA::ALSA)
endif(USE_ALSA)

if(USE_JACK)
    target_sources(anmp-internal PRIVATE AudioOutput/JackOutput.cpp AudioOutput/JackOutput.h)
    target_link_libraries(anmp-internal PUBLIC PkgConfig::JACK PkgConfig::SAMPLERATE)
endif(USE_JACK)

if(USE_PORTAUDIO)
    target_sources(anmp-internal PRIVATE AudioOutput/PortAudioOutput.cpp AudioOutput/PortAudioOutput.h)
    target_link_libraries(anmp-internal PUBLIC PkgConfig::PORTAUDIO)
endif(USE_PORTAUDIO)

if(EBUR128_FOUND)
    target_sources(anmp-internal PRIVATE AudioOutput/ebur128Output.cpp AudioOutput/ebur128Output.h)
    target_link_libraries(anmp-internal PUBLIC PkgConfig::EBUR128)
endif(EBUR128_FOUND)

if(USE_CUE)
  target_link_libraries(anmp-internal PUBLIC PkgConfig::LIBCUE)
endif()

if(LIBGME_FOUND)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/LibGMEWrapper.cpp InputLibraryWrapper/LibGMEWrapper.h)
endif(LIBGME_FOUND)

if(USE_MAD)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/LibMadWrapper.cpp InputLibraryWrapper/LibMadWrapper.h)
    target_link_libraries(anmp-internal PUBLIC PkgConfig::ID3TAG)
endif(USE_MAD)

if(LIBSND_FOUND)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/LibSNDWrapper.cpp InputLibraryWrapper/LibSNDWrapper.h)
    target_link_libraries(anmp-internal PUBLIC PkgConfig::LIBSND)
endif(LIBSND_FOUND)

if(USE_LAZYUSF)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/LazyusfWrapper.cpp InputLibraryWrapper/LazyusfWrapper.h)
endif(USE_LAZYUSF)

if(USE_AOPSF)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/AopsfWrapper.cpp InputLibraryWrapper/AopsfWrapper.h)
endif(USE_AOPSF)

if(VGMSTREAM_FOUND)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/VGMStreamWrapper.cpp InputLibraryWrapper/VGMStreamWrapper.h)
endif(VGMSTREAM_FOUND)

if(USE_FFMPEG)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/FFMpegWrapper.cpp InputLibraryWrapper/FFMpegWrapper.h)
    target_link_libraries(anmp-internal PUBLIC PkgConfig::AVCODEC PkgConfig::AVFORMAT PkgConfig::AVUTIL PkgConfig::SWRESAMPLE)
endif(USE_FFMPEG)

if(USE_SMF)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/MidiWrapper.cpp InputLibraryWrapper/MidiWrapper.h)
    target_link_libraries(anmp-internal PUBLIC PkgConfig::LIBSMF)
endif(USE_SMF)

if(USE_FLUIDSYNTH)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/FluidsynthWrapper.cpp InputLibraryWrapper/FluidsynthWrapper.h)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/N64CSeqWrapper.cpp InputLibraryWrapper/N64CSeqWrapper.h)
    target_link_libraries(anmp-internal PUBLIC PkgConfig::FLUIDSYNTH)
endif(USE_FLUIDSYNTH)

if(USE_MODPLUG)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/ModPlugWrapper.cpp InputLibraryWrapper/ModPlugWrapper.h)
endif(USE_MODPLUG)

if(USE_OPENMPT)
    target_sources(anmp-internal PRIVATE InputLibraryWrapper/OpenMPTWrapper.cpp InputLibraryWrapper/OpenMPTWrapper.h)
    target_link_libraries(anmp-internal PUBLIC PkgConfig::LIBOPENMPT)
endif(USE_OPENMPT)


add_library(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC anmp-internal)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LD_FLAGS})
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
