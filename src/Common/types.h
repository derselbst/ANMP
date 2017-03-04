#ifndef TYPES_H
#define TYPES_H

// declares commonly used types


#include <cstdint>

#include "AudioDriver.h"

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
enum class LoopType_t : uint8_t
{
    Forward,
    Backward,
    Alternating
};


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
    LoopType_t type = LoopType_t::Forward;

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


enum class SampleFormat_t : uint8_t
{
    unknown,
    uint8,
    int16,
    int24,
    int32,
    float32,
    float64,
};

#endif
