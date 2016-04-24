#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include "int24_t.hpp"

typedef void pcm_t;
typedef unsigned int frame_t;

typedef enum LoopType
{
    Forward,
    Backward,
    Alternating
} LoopType_t;


typedef struct loop
{
    // frame to start the loop
    frame_t start;

    // frame to end the loop
    // this frame WILL be played!
    frame_t stop;

    // how many times this loop is being played
    // zero indicates playing forever
    uint32_t count;

    LoopType_t type = LoopType::Forward;

} loop_t;


typedef enum AudioDriver
{
    AutoDetect,
    ALSA,
    JACK,
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