#include "IAudioOutput.h"

#include "CommonExceptions.h"


IAudioOutput::IAudioOutput()
{
    // by default assume that stereo output will be sent to hardware
    this->IAudioOutput::SetOutputChannels(2);
}

IAudioOutput::~IAudioOutput()
{}


void IAudioOutput::setVolume(float vol)
{
    this->volume=vol;
}

Nullable<uint16_t> IAudioOutput::GetOutputChannels()
{
    return this->outputChannels;
}

void IAudioOutput::SetOutputChannels(Nullable<uint16_t> chan)
{
    this->outputChannels = chan;
}

void IAudioOutput::SetVoiceConfig(decltype(SongFormat::Voices) voices, decltype(SongFormat::VoiceChannels)& voiceChannels)
{
    this->currentFormat.Voices = voices;
    this->currentFormat.VoiceChannels = voiceChannels;
}

void IAudioOutput::SetMuteMask(decltype(SongFormat::VoiceIsMuted)& mask)
{
    this->currentFormat.VoiceIsMuted = mask;
}

// takes care of pointer arithmetic
int IAudioOutput::write (const pcm_t* frameBuffer, frame_t frames, int offset)
{
    switch (this->currentFormat.SampleFormat)
    {
    case SampleFormat_t::float32:
    {
        const float* buf = static_cast<const float*>(frameBuffer);
        buf += offset;
        return this->write(buf, frames);
        break;
    }
    case SampleFormat_t::int16:
    {
        const int16_t* buf = static_cast<const int16_t*>(frameBuffer);
        buf += offset;
        return this->write(buf, frames);
        break;
    }
    case SampleFormat_t::int32:
    {
        const int32_t* buf = static_cast<const int32_t*>(frameBuffer);
        buf += offset;
        return this->write(buf, frames);
        break;
    }
    case SampleFormat_t::unknown:
        throw invalid_argument("pcmFormat mustnt be unknown");
        break;

    default:
        throw NotImplementedException();
        break;
    }
}











