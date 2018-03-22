
#include "AudioDriver.h"


const char *AudioDriverName[] =
{
#ifdef USE_ALSA
[Alsa] = "ALSA",
#endif

#ifdef USE_EBUR128
[Ebur128] = "ebur128 audio normalization",
#endif

#ifdef USE_JACK
[Jack] = "JACK",
#endif

#ifdef USE_PORTAUDIO
[Portaudio] = "PortAudio",
#endif

[Wave] = "write WAVE files",

[END] = "Srsly?",
};
