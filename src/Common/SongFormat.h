#ifndef SONGFORMAT_H
#define SONGFORMAT_H

#include "types.h"

#include <vector>

using std::vector;
using std::string;

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

    // specifies the number of (mono) audio channels
    unsigned int Channels();

    // indicates the type of the items in pcm buffer "data" from PCMHolder
    SampleFormat_t SampleFormat = SampleFormat_t::unknown;

    // number of voices available for a song
    // voice == group of consecutive audio channels
    unsigned int Voices = 0;
    
    vector<string> VoiceName;
    
    vector<bool> VoiceIsMuted;
    
    vector<unsigned int> VoiceChannels;

    void SetVoices(unsigned int nVoices);

    
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
