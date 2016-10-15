#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

/**
  * class Config
  * 
  * for doc see Config.cpp
  */

struct Config
{
    // Constructors/Destructors
    //
    // no object
    Config() = delete;
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

    static unsigned int defaultFadeTime;
    static unsigned int fadeTimeStop;
    static unsigned int fadeTimePause;

    /// USF specific section ***
    static bool useHle;

    static bool gmeAccurateEmulation;
    static unsigned int gmeSampleRate;
    static float gmeEchoDepth;
    static bool gmePlayForever;
    
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
};


#endif // CONFIG_H
