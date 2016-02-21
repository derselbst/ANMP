#ifndef SONGFORMAT_H
#define SONGFORMAT_H

#include <SampleFormat.h>

/**
  * class SongFormat
  * 
  */

struct SongFormat
{
    unsigned int SampleRate = 0;
    
    // indicates the type of the items in pcm buffer "data" from PCMHolder
    SampleFormat_t SampleFormat = SampleFormat_t::unknown;
    
    int Channels = 0;

    /**
     * returns bitrate in bit/s
     */
    void getBitrate ();

};

#endif // SONGFORMAT_H
