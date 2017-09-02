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
struct SongFormat
{
    // specifies the number of samples per seconds
    uint32_t SampleRate = 0;

    // specifies the number of (mono) audio channels
    uint32_t Channels() const noexcept;

    // indicates the type of the items in pcm buffer "data" from PCMHolder
    SampleFormat_t SampleFormat = SampleFormat_t::unknown;

    // number of voices available for a song
    // voice == group of consecutive audio channels
    uint16_t Voices = 0;
    
    vector<string> VoiceName;
    
    mutable vector<bool> VoiceIsMuted;
    
    mutable vector<uint16_t> VoiceChannels;

    void SetVoices(uint16_t nVoices);

    
    /**
     * returns bitrate in bit/s
     */
    int getBitrate () const;

    /**
     * whether this instance holds valid data
     */
    bool IsValid();
    
    /**
     * from \c nChannels create voices and assign each as many as \c defaultChannelsPerVoice channels
     * 
     * will do nothing if this.IsValid()==true (unless update \c forced)
     */
    void ConfigureVoices(const uint16_t nChannels, const uint16_t defaultChannelsPerVoice, bool force=false);

};

#endif // SONGFORMAT_H
