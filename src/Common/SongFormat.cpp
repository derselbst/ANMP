#include "SongFormat.h"

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

void SongFormat::ConfigureVoices(const uint16_t nChannels, const uint16_t defaultChannelsPerVoice, bool force)
{
    if(!force && this->IsValid())
    {
        // current voice config already valid, user might have changed it, do nothing if not forced
        return;
    }
    
    // by default group them to stereo voices
    int nvoices = nChannels / defaultChannelsPerVoice;
    int remainingvoices = (nChannels % defaultChannelsPerVoice == 0) ? 0 : 1;
    this->SetVoices(nvoices + remainingvoices);
    
    for(int i=0; i<nvoices; i++)
    {
        this->VoiceChannels[i] = defaultChannelsPerVoice; // == nChannels / nvoices
    }
    for(int i=0; i<remainingvoices; i++)
    {
        this->VoiceChannels[nvoices + i] = nChannels % defaultChannelsPerVoice;
    }
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

