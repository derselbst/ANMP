#ifndef TYPES_H
#define TYPES_H

// declares commonly used types


#include <cstdint>

// includes all headers with no corresponding cpp file

#include "CommonExceptions.h"
#include "Event.h"
#include "Nullable.h"
#include "tree.h"

// type used for all pcm buffers
// only IAudioOutput needs to know the specific type, determined by SampleFormat_t
typedef void pcm_t;

// usually stays unsigned, but for calculation purposes we better use signed
typedef int64_t frame_t;

// TODO: as of 2015-08-01 only Forward is used
// thus either implement the rest or remove me
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

    // as of 2015-08-01 never evaluated
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

static const char* AudioDriverName[] =
{
#ifdef USE_ALSA
    [AudioDriver_t::ALSA] = "ALSA",
#endif

#ifdef USE_EBUR128
    [AudioDriver_t::ebur128] = "ebur128 audio normalization",
#endif

#ifdef USE_JACK
    [AudioDriver_t::JACK] = "JACK",
#endif

#ifdef USE_PORTAUDIO
    [AudioDriver_t::PORTAUDIO] = "PortAudio",
#endif

    [AudioDriver_t::WAVE] = "write WAVE files",

    [AudioDriver_t::END] = "Srsly?",
};


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
