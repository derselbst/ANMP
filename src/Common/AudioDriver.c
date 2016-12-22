
#include "AudioDriver.h"


const char* AudioDriverName[] =
{
#ifdef USE_ALSA
    [ALSA] = "ALSA",
#endif

#ifdef USE_EBUR128
    [ebur128] = "ebur128 audio normalization",
#endif

#ifdef USE_JACK
    [JACK] = "JACK",
#endif

#ifdef USE_PORTAUDIO
    [PORTAUDIO] = "PortAudio",
#endif

    [WAVE] = "write WAVE files",

    [END] = "Srsly?",
};
