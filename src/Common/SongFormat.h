#ifndef SONGFORMAT_H
#define SONGFORMAT_H

#include "types.h"

/**
  * class SongFormat
  *
  */
struct SongFormat;
bool operator == (SongFormat const& lhs, SongFormat const& rhs);
bool operator != (SongFormat const& lhs, SongFormat const& rhs);

struct SongFormat
{
    // specifies the number of samples per seconds
    unsigned int SampleRate = 0;

    // specifies the number of (mono)-channels
    unsigned int Channels = 0;

    // indicates the type of the items in pcm buffer "data" from PCMHolder
    SampleFormat_t SampleFormat = SampleFormat_t::unknown;


    /**
     * returns bitrate in bit/s
     */
    void getBitrate ();

    /**
     * whether this instance holds valid data
     */
    bool IsValid();

};

#endif // SONGFORMAT_H
