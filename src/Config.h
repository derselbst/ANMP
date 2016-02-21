#ifndef CONFIG_H
#define CONFIG_H

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
    static AudioDriver_t audioDriver = AudioDriver_t::ALSA;
    static bool useLoopInfo = true;
    static int overridingGlobalLoopCount = -1;
    static const unsigned int FramesToRender = 2048;

};


#endif // CONFIG_H
