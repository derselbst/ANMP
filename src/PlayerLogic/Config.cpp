#include "Config.h"

    // Init Static Public attributes
    //
    AudioDriver_t Config::audioDriver = AudioDriver_t::ALSA;
    bool Config::useLoopInfo = true;
    int Config::overridingGlobalLoopCount = -1;

