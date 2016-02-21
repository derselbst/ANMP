#include <string>
#include <exception>
#include <stdexcept>

#define HAVE_ALSA_ASOUNDLIB_H 1

#if HAVE_ALSA_ASOUNDLIB_H
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>
#include <sys/time.h>
#endif

#if defined (__linux__)
#include     <fcntl.h>
#include     <sys/ioctl.h>
#include     <sys/soundcard.h>
#endif



#include "ALSAOutput.h"



// Constructors/Destructors
//  

ALSAOutput::ALSAOutput () {
}

ALSAOutput::~ALSAOutput () { }

//  
// Interface Implementation
//  
void ALSAOutput::open(bool realtime)
{  
  const char device[] = "default";

  if (alsa_dev==nullptr)
  {
      int err;

      if ((err = snd_pcm_open (&alsa_dev, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
      {
	  throw runtime_error("cannot open audio device \"" + string(device) + "\" (" + string(snd_strerror(err)) + ")");
      };

      snd_pcm_nonblock (alsa_dev, 0);

      snd_pcm_hw_params_t* hw_params;
      if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
      {
	  throw runtime_error("cannot allocate hardware parameter structure (" + string(string(snd_strerror(err))) + ")");
      };

      if ((err = snd_pcm_hw_params_any (alsa_dev, hw_params)) < 0)
      {
	  throw runtime_error("cannot initialize hardware parameter structure (" + string(string(snd_strerror(err))) + ")");
      };

      if ((err = snd_pcm_hw_params_set_access (alsa_dev, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
      {
	  throw runtime_error("cannot set access type (" + string(snd_strerror(err)) + ")");
      };

      snd_pcm_uframes_t alsa_period_size, alsa_buffer_frames;
      if (realtime)
      {   alsa_period_size = 256;
	  alsa_buffer_frames = 3 * alsa_period_size;
      }
      else
      {   alsa_period_size = 1024;
	  alsa_buffer_frames = 4 * alsa_period_size;
      };
      
      if ((err = snd_pcm_hw_params_set_buffer_size_near (alsa_dev, hw_params, &alsa_buffer_frames)) < 0)
      {
	  throw runtime_error("cannot set buffer size (" + string(snd_strerror(err)) + ")");
      };

      if ((err = snd_pcm_hw_params_set_period_size_near (alsa_dev, hw_params, &alsa_period_size, 0)) < 0)
      {
	  throw runtime_error("cannot set period size (" + string(snd_strerror(err)) + ")");
      };

      if ((err = snd_pcm_hw_params (alsa_dev, hw_params)) < 0)
      {
	  throw runtime_error("cannot set parameters (" + string(snd_strerror(err)) + ")");
      };

      /* extra check: if we have only one period, this code won't work */
      snd_pcm_hw_params_get_period_size (hw_params, &alsa_period_size, 0);
      
      snd_pcm_uframes_t buffer_size;
      snd_pcm_hw_params_get_buffer_size (hw_params, &buffer_size);
      if (alsa_period_size == buffer_size)
      {
	  throw runtime_error("Can't use period equal to buffer size (" + to_string(alsa_period_size) + " == " + to_string(buffer_size) + ")");
      };

      snd_pcm_hw_params_free (hw_params);

      snd_pcm_sw_params_t *sw_params;
      if ((err = snd_pcm_sw_params_malloc (&sw_params)) != 0)
      {
	  throw runtime_error(string(__func__) + ": snd_pcm_sw_params_malloc: " + string(snd_strerror(err)));
      };

      if ((err = snd_pcm_sw_params_current (alsa_dev, sw_params)) != 0)
      {
	  throw runtime_error(string(__func__) + ": snd_pcm_sw_params_current: " + string(snd_strerror(err)));
      };

      /* note: set start threshold to delay start until the ring buffer is full */
      if ((err = snd_pcm_sw_params_set_start_threshold (alsa_dev, sw_params, buffer_size)) < 0)
      {
	  throw runtime_error("cannot set start threshold (" + string(snd_strerror(err)) + ")");
      };

      if ((err = snd_pcm_sw_params (alsa_dev, sw_params)) != 0)
      {
	  throw runtime_error(string(__func__) + ": snd_pcm_sw_params: " + string(snd_strerror(err)));
      };

      snd_pcm_sw_params_free (sw_params);

      snd_pcm_reset (alsa_dev);
  }
} /* alsa_open */

void ALSAOutput::init(unsigned int sampleRate, uint8_t channels, SampleFormat_t s)
{
  // changing hw settings can only safely be done when pcm is not running
  // therefore stop the pcm and drop all pending frames
  this->drop();
  
  int err;
  
    snd_pcm_hw_params_t* hw_params;
    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
    {
        throw runtime_error("cannot allocate hardware parameter structure (" + string(snd_strerror(err)) + ")");
    };

    if ((err = snd_pcm_hw_params_current (alsa_dev, hw_params)) < 0)
    {
	snd_pcm_hw_params_free (hw_params);
        throw runtime_error("cannot initialize hardware parameter structure with current settings (" + string(snd_strerror(err)) + ")");
    };
  
    // TODO: set sample format according to s
    if ((err = snd_pcm_hw_params_set_format (alsa_dev, hw_params, SND_PCM_FORMAT_FLOAT)) < 0)
    {
	snd_pcm_hw_params_free (hw_params);
        throw runtime_error("cannot set sample format (" + string(snd_strerror(err)) + ")");
    };
    
    if ((err = snd_pcm_hw_params_set_rate_near (alsa_dev, hw_params, &sampleRate, 0)) < 0)
    {
	snd_pcm_hw_params_free (hw_params);
        throw runtime_error("cannot set sample rate (" + string(snd_strerror(err)) + ")");
    };

    if ((err = snd_pcm_hw_params_set_channels (alsa_dev, hw_params, channels)) < 0)
    {
	snd_pcm_hw_params_free (hw_params);
        throw runtime_error("cannot set channel count (" + string(snd_strerror(err)) + ")");
    };
    
    if ((err = snd_pcm_hw_params (alsa_dev, hw_params)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        throw runtime_error("cannot set parameters (" + string(snd_strerror(err)) + ")");
    };
    
    snd_pcm_hw_params_free (hw_params);
    
    // restart pcm
    this->start();
    
    this->lastChannelCount = channels;
}

void ALSAOutput::drain()
{
  snd_pcm_drain(alsa_dev);
}

void ALSAOutput::drop()
{
  snd_pcm_drop(alsa_dev);
}

void ALSAOutput::close()
{
    if (alsa_dev != nullptr)
    { 
	snd_pcm_close (alsa_dev);
    };
}

void ALSAOutput::write (float* buffer, unsigned int frames)
{
  
}

void ALSAOutput::write (int16_t* buffer, unsigned int frames)
{
  
}

void ALSAOutput::write (pcm_t* frameBuffer, unsigned int frames, int offset, SampleFormat_t pcmFormat)
{
  
}

// Accessor methods
//  


// Other methods
//  


void ALSAOutput::start()
{
                int err = snd_pcm_start(alsa_dev);
                if (err < 0) {
                        //fprintf (stderr,"Start PCM error: %s\n", string(snd_strerror(err)));
                }
}

