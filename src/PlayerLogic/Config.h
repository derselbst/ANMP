#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

/**
  * class Config
  *
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

    // Static Public attributes
    //
    static AudioDriver_t audioDriver;
    static bool useLoopInfo;
    static int overridingGlobalLoopCount;
    static const frame_t FramesToRender;
    static unsigned int PreRenderTime;
    static bool RenderWholeSong;

    static unsigned int defaultFadeTime;
    static unsigned int fadeTimeStop;
    static unsigned int fadeTimePause;

    /// USF specific section ***
    static bool useHle;

    static bool gmeAccurateEmulation;
    static unsigned int gmeSampleRate;
    static float gmeEchoDepth;
    static bool gmePlayForever;

};


#endif // CONFIG_H
