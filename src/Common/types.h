#pragma once

// declares commonly used types

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#define restrict __restrict__
#else
#define restrict
#endif


#include <cstdint>

// includes all headers with no corresponding cpp file
#include "AudioDriver.h"
#include "CommonExceptions.h"
#include "Event.h"
#include "Nullable.h"
#include "SampleFormat.h"
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

// depending whether the audio file we contains floats, doubles or ints, access the decoded pcm as int or float
union sndfile_sample_t
{
    // classes from the outside will either see an int32 or a float here. they dont see this union. they cant access the members via f and i members. we require float to be 32 bits, to avoid having any padding bits that would be treated as audio.
    static_assert(sizeof(float) == sizeof(int32_t), "sizeof(float) != sizeof(int32_t), sry, this code wont work");

    float f;
    int32_t i;
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

    friend bool operator<(struct loop const &lhs, struct loop const &rhs)
    {
        return lhs.start < rhs.start && lhs.stop <= rhs.start;
    }

    friend bool operator==(struct loop const &lhs, struct loop const &rhs)
    {
        return lhs.start == rhs.start &&
               lhs.stop == rhs.stop &&
               lhs.count == rhs.count &&
               lhs.type == rhs.type;
    }

} loop_t;
