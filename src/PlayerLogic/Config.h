#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

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

struct Config
{
    // Constructors/Destructors
    //
    // no object
    Config() {}
    // no copy
    Config(const Config&) = delete;
    // no assign
    Config& operator=(const Config&) = delete;


    static constexpr frame_t FramesToRender = 2048;

    // Static Public attributes
    //
    static AudioDriver_t audioDriver;
    static bool useLoopInfo;
    static int overridingGlobalLoopCount;
    static unsigned int PreRenderTime;
    static bool RenderWholeSong;
    static bool useAudioNormalization;

    static unsigned int fadeTimeStop;
    static unsigned int fadeTimePause;

    /// USF specific section ***
    static bool useHle;

    static bool gmeAccurateEmulation;
    static unsigned int gmeSampleRate;
    static float gmeEchoDepth;
    static bool gmePlayForever;

    static uint8_t MidiControllerLoopStart;
    static uint8_t MidiControllerLoopStop;

    static int FluidsynthPeriodSize;
    static bool FluidsynthEnableReverb;
    static bool FluidsynthEnableChorus;
    static bool FluidsynthMultiChannel;
    static unsigned int FluidsynthSampleRate;
    static bool FluidsynthForceDefaultSoundfont;
    static string FluidsynthDefaultSoundfont;
    static double FluidsynthRoomSize;
    static double FluidsynthDamping;
    static int FluidsynthWidth;
    static double FluidsynthLevel;
    
    template<class Archive>
    static void serialize(Archive& archive, const uint32_t version)
    {
        switch(version)
        {
            case 1:
                archive( CEREAL_NVP(Config::audioDriver) );
                archive( CEREAL_NVP(Config::useLoopInfo) );
                archive( CEREAL_NVP(Config::overridingGlobalLoopCount) );
                archive( CEREAL_NVP(Config::PreRenderTime) );
                archive( CEREAL_NVP(Config::RenderWholeSong) );
                archive( CEREAL_NVP(Config::useAudioNormalization) );

                archive( CEREAL_NVP(Config::fadeTimeStop) );
                archive( CEREAL_NVP(Config::fadeTimePause) );

                archive( CEREAL_NVP(Config::useHle) );

                archive( CEREAL_NVP(Config::gmeAccurateEmulation) );
                archive( CEREAL_NVP(Config::gmeSampleRate) );
                archive( CEREAL_NVP(Config::gmeEchoDepth) );
                archive( CEREAL_NVP(Config::gmePlayForever) );

                archive( CEREAL_NVP(Config::MidiControllerLoopStart) );
                archive( CEREAL_NVP(Config::MidiControllerLoopStop) );

                archive( CEREAL_NVP(Config::FluidsynthPeriodSize) );
                archive( CEREAL_NVP(Config::FluidsynthEnableReverb) );
                archive( CEREAL_NVP(Config::FluidsynthEnableChorus) );
                archive( CEREAL_NVP(Config::FluidsynthMultiChannel) );
                archive( CEREAL_NVP(Config::FluidsynthSampleRate) );
                archive( CEREAL_NVP(Config::FluidsynthForceDefaultSoundfont) );
                archive( CEREAL_NVP(Config::FluidsynthDefaultSoundfont) );
                archive( CEREAL_NVP(Config::FluidsynthRoomSize) );
                archive( CEREAL_NVP(Config::FluidsynthDamping) );
                archive( CEREAL_NVP(Config::FluidsynthWidth) );
                archive( CEREAL_NVP(Config::FluidsynthLevel) );
                break;
            default:
                throw InvalidVersionException(version);
        }
    }
};

CEREAL_CLASS_VERSION( Config, 1 );


#endif // CONFIG_H
