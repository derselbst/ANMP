#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

#include <string>
#include <cereal/cereal.hpp>

/**
  * class Config
  *
  * holds configuration parameters that determine various aspects of how audio files are being decoded/played
  *
  * if not stated else, these settings will take effect after the next call to Song::open() (assuming the Song was Song::close() before)
  *
  * for var-specific doc see Config.cpp
  */

class Config
{
    // Constructors/Destructors
    //
  
    Config();
    
    // no copy
    Config(const Config&) = delete;
    // no assign
    Config& operator=(const Config&) = delete;

public:

    static Config& Singleton();
    
    //**********************************
    //     GLOBAL INTERNAL SECTION     *
    //**********************************

    // number of frames that are pushed to audioDriver during each run AND minimum no. of frames
    // that have to be prepared (i.e. rendered) by a single call to Song::fillBuffer()
    static constexpr frame_t FramesToRender = 2048;
    
    // indicates the default audio driver to use
    // you have to call Player::initAudio() for changes to take effect
    AudioDriver_t audioDriver = AudioDriver_t::ALSA;

    // in synchronous part of Song::fillBuffer(): prepare as many samples as needed to have enough for
    // PreRenderTime of playing back duration, before asynchronously filling up the rest of PCM buffer
    //
    // time in milliseconds
    //
    // high values will slow down the start of playing back the next song, but will assure that the
    // asynchronous part of Song::fillBuffer() has enough headstart (i.e. the PCM buffer is well filled)
    // so there are no audible glitches later on (such as parts of old songs or cracks and clicks...)
    unsigned int PreRenderTime = 500;

    // indicates whether the currently playing audiofile shall be only decoded once and held in memory as a whole (true)
    // or if only a small buffer shall be allocated holding only FramesToRender frames at one time
    // can be set to false, if user needs to save memory, however this will also make seeking within the file impossible
    bool RenderWholeSong = true;

    // whether to use the audio normalization information generated by anmp-normalize or not
    bool useAudioNormalization = true;

    //**********************************
    //       HOW-TO-PLAY SECTION       *
    //**********************************

    // whether the loop information a track may contain, shall be used
    bool useLoopInfo = true;

    // play each and every loop that many time, -1 will ignore this setting
    int overridingGlobalLoopCount = -1;

    // time in milliseconds to fade out a song when the user hits stop
    unsigned int fadeTimeStop = 3500;

    // time in milliseconds to fade out a song when the user hits pause
    unsigned int fadeTimePause = 300;

    //**********************************
    //      USF-SPECIFIC SECTION       *
    //**********************************

    // whether to use High-Level-Emulation (HLE) or not (i.e. Low-Level-Emulation (LLE))
    // true: speed up emulation (about factor 2 or more) at the cost of accuracy and potential emulation bugs
    // unless you are audiophil, there is no reason to set this to false, since HLE became pretty accurate over the last years
    // however you should definitly set this to true when being on a mobile device
    bool useHle = false;

    //**********************************
    //    LIBGME-SPECIFIC SECTION      *
    //**********************************

    unsigned int gmeSampleRate = 48000;

    bool gmeAccurateEmulation = true;

    /* Adjust stereo echo depth, where 0.0 = off and 1.0 = maximum. Has no effect for
    GYM, SPC, and Sega Genesis VGM music */
    float gmeEchoDepth = 0.2f;

    bool gmePlayForever = false;

    //**********************************
    //      MIDI-GENERAL SECTION       *
    //**********************************

    // no. of the MIDI CC that indicates the start of a looped section
    uint8_t MidiControllerLoopStart = 102;

    // no. of the MIDI CC that indicates the stop of a looped section
    uint8_t MidiControllerLoopStop = 103;

    //**********************************
    //   FLUIDSYNTH-SPECIFIC SECTION   *
    //**********************************

    // overrides FramesToRender for fluidsynth
    int FluidsynthPeriodSize = 64;

    // enables reverb in fluidsynth's synth
    bool FluidsynthEnableReverb = true;

    // enables chorus in fluidsynth's synth
    bool FluidsynthEnableChorus = true;

    // write each midi channel to a separate stereo channel (i.e. 32 channel raw pcm channels)
    // only use this when writing to file or maybe when using jackd
    bool FluidsynthMultiChannel = false;

    // the sample rate at with fluidsynth will synthesize
    unsigned int FluidsynthSampleRate = 48000;

    // always use the default soundfont
    bool FluidsynthForceDefaultSoundfont = false;

    // the soundfont to use, if there are no more suitable soundfonts found
    // this should be a GM Midi conform SF2
    string FluidsynthDefaultSoundfont = "/home/tom/Musik/Banjo-Kazooie [Banjo to Kazooie no Daibouken] (1998-05-31)(Rare)(Nintendo)/BK.sf2";

    // parameters for fluidsynth's reverb effect
    double FluidsynthRoomSize = 0.8;
    double FluidsynthDamping = 0.01;
    int FluidsynthWidth = 0;
    double FluidsynthLevel = 0.8;
    
    
    template<class Archive>
    void serialize(Archive& archive, const uint32_t version)
    {
        switch(version)
        {
            case 1:
                archive( CEREAL_NVP(this->audioDriver) );
                archive( CEREAL_NVP(this->useLoopInfo) );
                archive( CEREAL_NVP(this->overridingGlobalLoopCount) );
                archive( CEREAL_NVP(this->PreRenderTime) );
                archive( CEREAL_NVP(this->RenderWholeSong) );
                archive( CEREAL_NVP(this->useAudioNormalization) );

                archive( CEREAL_NVP(this->fadeTimeStop) );
                archive( CEREAL_NVP(this->fadeTimePause) );

                archive( CEREAL_NVP(this->useHle) );

                archive( CEREAL_NVP(this->gmeAccurateEmulation) );
                archive( CEREAL_NVP(this->gmeSampleRate) );
                archive( CEREAL_NVP(this->gmeEchoDepth) );
                archive( CEREAL_NVP(this->gmePlayForever) );

                archive( CEREAL_NVP(this->MidiControllerLoopStart) );
                archive( CEREAL_NVP(this->MidiControllerLoopStop) );

                archive( CEREAL_NVP(this->FluidsynthPeriodSize) );
                archive( CEREAL_NVP(this->FluidsynthEnableReverb) );
                archive( CEREAL_NVP(this->FluidsynthEnableChorus) );
                archive( CEREAL_NVP(this->FluidsynthMultiChannel) );
                archive( CEREAL_NVP(this->FluidsynthSampleRate) );
                archive( CEREAL_NVP(this->FluidsynthForceDefaultSoundfont) );
                archive( CEREAL_NVP(this->FluidsynthDefaultSoundfont) );
                archive( CEREAL_NVP(this->FluidsynthRoomSize) );
                archive( CEREAL_NVP(this->FluidsynthDamping) );
                archive( CEREAL_NVP(this->FluidsynthWidth) );
                archive( CEREAL_NVP(this->FluidsynthLevel) );
                break;
            default:
                throw InvalidVersionException(version);
        }
    }
};

CEREAL_CLASS_VERSION( Config, 1 )

// global var holding the singleton Config instance
// just a nice little shortcut, so one doesnt always have to write Config::Singleton()
extern Config& gConfig;

#endif // CONFIG_H
