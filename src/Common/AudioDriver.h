
#pragma once


#ifdef __cplusplus
extern "C"
{
#endif

enum
#ifdef __cplusplus
class // c99 doesnt support scoped enums, fallback to unscoped ones
#endif
AudioDriver_t
{
    BEGIN = 0,
    Wave = BEGIN,

#ifdef USE_ALSA
    Alsa,
#endif

#ifdef USE_JACK
    Jack,
#endif

#ifdef USE_PORTAUDIO
    Portaudio,
#endif

#ifdef USE_EBUR128
    Ebur128,
#endif

    END,
};

extern const char* AudioDriverName[];

#ifdef __cplusplus
}
#endif