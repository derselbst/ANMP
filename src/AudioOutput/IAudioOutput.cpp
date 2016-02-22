
#include "IAudioOutput.h"


int IAudioOutput::write (pcm_t* frameBuffer, unsigned int frames, int offset, SampleFormat_t pcmFormat)
{
  switch (pcmFormat)
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