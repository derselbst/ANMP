
#pragma once

typedef enum AudioDriver
{
    BEGIN,
    WAVE = BEGIN,

#ifdef USE_ALSA
    ALSA,
#endif

#ifdef USE_JACK
    JACK,
#endif

#ifdef USE_PORTAUDIO
    PORTAUDIO,
#endif

#ifdef USE_EBUR128
    ebur128,
#endif

    END,
} AudioDriver_t;

extern const char* AudioDriverName[];
