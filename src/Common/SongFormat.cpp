#include "SongFormat.h"

bool operator == (SongFormat const& lhs, SongFormat const& rhs)
{
    return lhs.SampleRate  == rhs.SampleRate   &&
           lhs.SampleFormat== rhs.SampleFormat &&
           lhs.Channels()    == rhs.Channels();
}

bool operator != (SongFormat const& lhs, SongFormat const& rhs)
{
    return !(lhs == rhs);
}

unsigned int SongFormat::Channels() const noexcept
{
    unsigned int sum = 0;
    for(unsigned int i=0; i<this->Voices; i++)
    {
        sum += this->VoiceChannels[i];
    }
    return sum;
}

void SongFormat::SetVoices(uint16_t nVoices)
{
    if(nVoices == 0)
    {
        this->VoiceName.clear();
        this->VoiceName.shrink_to_fit();
        this->VoiceIsMuted.clear();
        this->VoiceIsMuted.shrink_to_fit();
        this->VoiceChannels.clear();
        this->VoiceChannels.shrink_to_fit();
    }
    else
    {
        this->VoiceName.resize(nVoices);
        this->VoiceIsMuted.resize(nVoices, false);
        this->VoiceChannels.resize(nVoices, 0);
    }
    
    for(uint16_t i=0; i<nVoices; i++)
    {
        this->VoiceName[i] = "Voice " + to_string(i);
    }
    
    this->Voices = nVoices;
}


/**
 * returns bitrate in bit/s
 */
void SongFormat::getBitrate ()
{
// samplerate * sample depth * channels
}

bool SongFormat::IsValid()
{
    return this->SampleRate != 0 && this->Channels() != 0 && this->SampleFormat != SampleFormat_t::unknown;
}

