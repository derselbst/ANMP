#include "IAudioOutput.h"

#include "CommonExceptions.h"


IAudioOutput::IAudioOutput()
{}

IAudioOutput::~IAudioOutput()
{}


void IAudioOutput::setVolume(float vol)
{
    this->volume=vol;
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











