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
public:
    Config();

    // returns ANMP's currently active configuration
    static Config& Singleton();
    
    //**********************************
    //     GLOBAL INTERNAL SECTION     *
    //**********************************
    
    static constexpr const char UserDir[] = ".anmp";
    static constexpr const char UserFile[] = "config.json";

    // number of frames that are pushed to audioDriver during each run AND minimum no. of frames
    // that have to be prepared (i.e. rendered) by a single call to Song::fillBuffer()
    static constexpr frame_t FramesToRender = 2048;
    
    // indicates the default audio driver to use
    // you have to call Player::initAudio() for changes to take effect
    AudioDriver_t audioDriver = AudioDriver_t::Alsa;

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
    
    bool gmeMultiChannel = false;

    //**********************************
    //      MIDI-GENERAL SECTION       *
    //**********************************

    // no. of the MIDI CC that indicates the start of a looped section
    uint8_t MidiControllerLoopStart = 102;

    // no. of the MIDI CC that indicates the stop of a looped section
    uint8_t MidiControllerLoopStop = 103;
    
    // no. of the MIDI CC that indicates the loop count of looped sections
    uint8_t MidiControllerLoopCount = 104;

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

    // package builders may define this, to default to a bundled soundfont
    #ifndef FLUIDSYNTH_DEFAULT_SF2
        #define FLUIDSYNTH_DEFAULT_SF2 ""
    #endif
    // the soundfont to use, if there are no more suitable soundfonts found
    // this should be a GM Midi conform SF2
    string FluidsynthDefaultSoundfont = FLUIDSYNTH_DEFAULT_SF2;

    // parameters for fluidsynth's reverb effect
    double FluidsynthRoomSize = 0.8;
    double FluidsynthDamping = 0.01;
    int FluidsynthWidth = 0;
    double FluidsynthLevel = 0.8;
    
    string FluidsynthBankSelect = "mma";
    bool FluidsynthChannel9IsDrum = false;
    
    
    //**********************************
    //   LIBMODPLUG-SPECIFIC SECTION   *
    //**********************************
    
    bool ModPlugEnableNoiseRed = false;
    
    bool ModPlugEnableReverb = false;
    
    bool ModPlugEnableBass = false;
    
    bool ModPlugEnableSurround = false;
    
    int ModPlugSampleRate = 44100;
    
    // Reverb level 0(quiet)-100(loud)
    int ModPlugReverbDepth = 50;
    
    // Reverb delay in ms, usually 40-200ms
    int ModPlugReverbDelay = 100;
    
    // XBass level 0(quiet)-100(loud)
    int ModPlugBassAmount = 25;
    
    // XBass cutoff in Hz 10-100
    int ModPlugBassRange = 60;
    
    // Surround level 0(quiet)-100(heavy)
    int ModPlugSurroundDepth = 50;
    
    // Surround delay in ms, usually 5-40ms
    int ModPlugSurroundDelay = 10;
    
    //**********************************
    //   LIBMAD-SPECIFIC SECTION   *
    //**********************************
    bool MadPermissive = false;
    
    
    void Load() noexcept;
    void Save() noexcept;
    
    template<class Archive>
    void serialize(Archive& archive, const uint32_t version)
    {
        switch(version)
        {
            case 4:
                archive( CEREAL_NVP(this->MadPermissive) );
                archive( CEREAL_NVP(this->FluidsynthBankSelect) );
                archive( CEREAL_NVP(this->FluidsynthChannel9IsDrum) );
                [[fallthrough]];
            case 3: // added MidiControllerLoopCount
                archive( CEREAL_NVP(this->MidiControllerLoopCount) );
                [[fallthrough]];
            case 2: // only added modplug variables and gmeMultiChannel, rest is the same as in version 1
                archive( CEREAL_NVP(this->gmeMultiChannel) );
                
                archive( CEREAL_NVP(this->ModPlugEnableNoiseRed) );
                archive( CEREAL_NVP(this->ModPlugEnableReverb) );
                archive( CEREAL_NVP(this->ModPlugEnableBass) );
                archive( CEREAL_NVP(this->ModPlugEnableSurround) );
                archive( CEREAL_NVP(this->ModPlugSampleRate) );
                archive( CEREAL_NVP(this->ModPlugReverbDepth) );
                archive( CEREAL_NVP(this->ModPlugReverbDelay) );
                archive( CEREAL_NVP(this->ModPlugBassAmount) );
                archive( CEREAL_NVP(this->ModPlugBassRange) );
                archive( CEREAL_NVP(this->ModPlugSurroundDepth) );
                archive( CEREAL_NVP(this->ModPlugSurroundDelay) );
                [[fallthrough]];
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

CEREAL_CLASS_VERSION( Config, 4 )

// global var holding the singleton Config instance
// just a nice little shortcut, so one doesnt always have to write Config::Singleton()
extern Config& gConfig;

#endif // CONFIG_H
