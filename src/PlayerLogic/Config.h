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
    static unsigned int defaultFadeTime;


    /// USF specific section ***
    static bool useHle;

};


#endif // CONFIG_H
