#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include "int24_t.hpp"

typedef void pcm_t;

// usually stays unsigned, but for calculation purposes we better use signed
typedef int64_t frame_t;

typedef enum LoopType
{
    Forward,
    Backward,
    Alternating
} LoopType_t;


typedef struct loop
{
    // frame to start the loop
    frame_t start = 0;

    // frame to end the loop
    // this frame WILL be played!
    frame_t stop = 0;

    // how many times this loop is being played
    // zero indicates playing forever
    uint32_t count = 0;

    // never evaluated
    LoopType_t type = LoopType::Forward;

    friend bool operator < (struct loop const& lhs, struct loop const& rhs)
    {
        return lhs.start < rhs.start && lhs.stop <= rhs.start;
    }

    friend bool operator == (struct loop const& lhs, struct loop const& rhs)
    {
        return lhs.start == rhs.start &&
               lhs.stop  == rhs.stop  &&
               lhs.count == rhs.count &&
               lhs.type  == rhs.type;
    }

} loop_t;


typedef enum AudioDriver
{
#ifdef USE_ALSA
    ALSA,
#endif
#ifdef USE_EBUR128
    ebur128,
#endif
#ifdef USE_PORTAUDIO
    PORTAUDIO,
#endif
    WAVE,
} AudioDriver_t;


typedef enum SampleFormat
{
    unknown,
    uint8,
    int16,
    int24,
    int32,
    float32,
    float64,
} SampleFormat_t;

#endif
