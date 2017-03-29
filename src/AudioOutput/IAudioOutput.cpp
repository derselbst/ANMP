#include "IAudioOutput.h"

#include "CommonExceptions.h"


IAudioOutput::IAudioOutput()
{
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

void IAudioOutput::stop()
{
    // because: playback gets stopped, currentSong gets deleted, new song gets allocated at that memory that has just been freed
    // some child classes compare cached format (this->currentFormat) to newSong.Format, if they equal they avoid unecessary init to increase performance
    // to avoid potential access via dangling, set it to null when not playing
    this->currentFormat = nullptr;
}

// takes care of pointer arithmetic
int IAudioOutput::write (const pcm_t* frameBuffer, frame_t frames, int offset)
{
    switch (this->currentFormat->SampleFormat)
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











