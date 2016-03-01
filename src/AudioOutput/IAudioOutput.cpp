
#include <exception>
#include <stdexcept>

#include "CommonExceptions.h"
#include "IAudioOutput.h"

IAudioOutput::IAudioOutput()
{}

IAudioOutput::~IAudioOutput()
{}


int IAudioOutput::write (pcm_t* frameBuffer, frame_t frames, int offset)
{
    switch (this->currentSampleFormat)
    {
    case float32:
    {
        float* buf = static_cast<float*>(frameBuffer);
        buf += offset;
        return this->write(buf, frames);
        break;
    }
    case int16:
    {
        int16_t* buf = static_cast<int16_t*>(frameBuffer);
        buf += offset;
        return this->write(buf, frames);
        break;
    }
    case unknown:
        throw invalid_argument("pcmFormat mustnt be unknown");
        break;

    default:
        throw NotImplementedException();
        break;
    }
}