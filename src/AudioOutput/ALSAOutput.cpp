#include <string>

#include "CommonExceptions.h"
#include "ALSAOutput.h"



// Constructors/Destructors
//  

ALSAOutput::ALSAOutput () {
}

ALSAOutput::~ALSAOutput ()
{
  this->drop();
  this->close();
}

//  
// Interface Implementation
//  
void ALSAOutput::open()
{  
  const char device[] = "default";

  if (this->alsa_dev==nullptr)
  {
    int err;
      if ((err = snd_pcm_open (&this->alsa_dev, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
      {
	this->alsa_dev=nullptr;
	  throw runtime_error("cannot open audio device \"" + string(device) + "\" (" + string(snd_strerror(err)) + ")");
      }
  }
} /* alsa_open */

void ALSAOutput::init(unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime)
{
  // changing hw settings can only safely be done when pcm is not running
  // therefore stop the pcm and drain all pending frames
  // DO NOT drop pending frames, due to latency some frames might still be played,
  // dropping them will cause (at least in non-realtime mode) a hearable brick
  this->drain();
  
  int err;

  // block when calling ALSA API
  snd_pcm_nonblock (this->alsa_dev, 0);

      snd_pcm_hw_params_t* hw_params;
      if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
      {
	  throw runtime_error("cannot allocate hardware parameter structure (" + string(string(snd_strerror(err))) + ")");
      }

      if ((err = snd_pcm_hw_params_any (this->alsa_dev, hw_params)) < 0)
      {
	snd_pcm_hw_params_free (hw_params);
	  throw runtime_error("cannot initialize hardware parameter structure (" + string(string(snd_strerror(err))) + ")");
      }

      if ((err = snd_pcm_hw_params_set_access (this->alsa_dev, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
      {
	snd_pcm_hw_params_free (hw_params);
	  throw runtime_error("cannot set access type (" + string(snd_strerror(err)) + ")");
      }

/***************
 * INTERESTING *
 **************/
    switch(s)
    {
      case float32:
	err = snd_pcm_hw_params_set_format (this->alsa_dev, hw_params, SND_PCM_FORMAT_FLOAT);
	break;
      case int16:
	err = snd_pcm_hw_params_set_format (this->alsa_dev, hw_params, SND_PCM_FORMAT_S16);
	break;
      case unknown:
	throw runtime_error("ALSAOutput::init(): Sample Format not set");
      default:
	throw NotImplementedException();
	break;
    }
    if (err < 0)
    {
	snd_pcm_hw_params_free (hw_params);
        throw runtime_error("cannot set sample format (" + string(snd_strerror(err)) + ")");
    }
    
    if ((err = snd_pcm_hw_params_set_rate_near (this->alsa_dev, hw_params, &sampleRate, 0)) < 0)
    {
	snd_pcm_hw_params_free (hw_params);
        throw runtime_error("cannot set sample rate (" + string(snd_strerror(err)) + ")");
    }

    if ((err = snd_pcm_hw_params_set_channels (this->alsa_dev, hw_params, channels)) < 0)
    {
	snd_pcm_hw_params_free (hw_params);
        throw runtime_error("cannot set channel count (" + string(snd_strerror(err)) + ")");
    }
// end interesting

    snd_pcm_uframes_t alsa_period_size, alsa_buffer_frames;
      if (realtime)
      {   alsa_period_size = 256;
	  alsa_buffer_frames = 3 * alsa_period_size;
      }
      else
      {   alsa_period_size = 1024;
	  alsa_buffer_frames = 4 * alsa_period_size;
      }
      
      if ((err = snd_pcm_hw_params_set_buffer_size_near (this->alsa_dev, hw_params, &alsa_buffer_frames)) < 0)
      {
	snd_pcm_hw_params_free (hw_params);
	  throw runtime_error("cannot set buffer size (" + string(snd_strerror(err)) + ")");
      }

      if ((err = snd_pcm_hw_params_set_period_size_near (this->alsa_dev, hw_params, &alsa_period_size, 0)) < 0)
      {
	snd_pcm_hw_params_free (hw_params);
	  throw runtime_error("cannot set period size (" + string(snd_strerror(err)) + ")");
      }

    if ((err = snd_pcm_hw_params (this->alsa_dev, hw_params)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        throw runtime_error("cannot set parameters (" + string(snd_strerror(err)) + ")");
    }
    

      /* extra check: if we have only one period, this code won't work */
      snd_pcm_hw_params_get_period_size (hw_params, &alsa_period_size, 0);
      
      snd_pcm_uframes_t buffer_size;
      snd_pcm_hw_params_get_buffer_size (hw_params, &buffer_size);
      if (alsa_period_size == buffer_size)
      {
	snd_pcm_hw_params_free (hw_params);
	  throw runtime_error("Can't use period equal to buffer size (" + to_string(alsa_period_size) + " == " + to_string(buffer_size) + ")");
      }

      snd_pcm_hw_params_free (hw_params);


      snd_pcm_sw_params_t *sw_params;
      if ((err = snd_pcm_sw_params_malloc (&sw_params)) != 0)
      {
	  throw runtime_error(string(__func__) + ": snd_pcm_sw_params_malloc: " + string(snd_strerror(err)));
      }

      if ((err = snd_pcm_sw_params_current (this->alsa_dev, sw_params)) != 0)
      {
	snd_pcm_sw_params_free (sw_params);
	  throw runtime_error(string(__func__) + ": snd_pcm_sw_params_current: " + string(snd_strerror(err)));
      }

      /* note: set start threshold to delay start until the ring buffer is full */
      if ((err = snd_pcm_sw_params_set_start_threshold (this->alsa_dev, sw_params, buffer_size)) < 0)
      {
	snd_pcm_sw_params_free (sw_params);
	  throw runtime_error("cannot set start threshold (" + string(snd_strerror(err)) + ")");
      }

      if ((err = snd_pcm_sw_params (this->alsa_dev, sw_params)) != 0)
      {
	snd_pcm_sw_params_free (sw_params);
	  throw runtime_error(string(__func__) + ": snd_pcm_sw_params: " + string(snd_strerror(err)));
      }

      snd_pcm_sw_params_free (sw_params);

      snd_pcm_reset (this->alsa_dev);
    
      // pcm might still be in stopped state, start it explicitly
    this->start();
    
    // WOW, WE MADE IT TIL HERE, so update channelcount and srate
    this->currentChannelCount = channels;
    this->currentSampleFormat = s;
}

void ALSAOutput::drain()
{
  snd_pcm_drain(this->alsa_dev);
}

void ALSAOutput::drop()
{
  snd_pcm_drop(this->alsa_dev);
}

void ALSAOutput::close()
{
    if (this->alsa_dev != nullptr)
    { 
	snd_pcm_close (this->alsa_dev);
	this->alsa_dev = nullptr;
    }
}

int ALSAOutput::write (float* buffer, unsigned int frames)
{
  static int epipe_count = 0;

    if (epipe_count > 0)
    {
        epipe_count--;
    }
    
int total = 0;
    while (total < frames)
    { 
      int retval = snd_pcm_writei (this->alsa_dev, buffer + total * this->currentChannelCount, frames - total);

        if (retval >= 0)
        {   total += retval;
            if (total == frames)
                return total;

            continue;
        };

        switch (retval)
        {
        case -EAGAIN:
            puts ("alsa_write: EAGAIN");
            continue;
            break;

        case -EPIPE:
            if (epipe_count > 0)
            {   printf ("alsa_write: EPIPE %d\n", epipe_count);
                if (epipe_count > 140)
                    return retval;
            };
            epipe_count += 100;

#if 0
            if (0)
            {   snd_pcm_status_t *status;

                snd_pcm_status_alloca (&status);
                if ((retval = snd_pcm_status (alsa_dev, status)) < 0)
                    fprintf (stderr, "alsa_out: xrun. can't determine length\n");
                else if (snd_pcm_status_get_state (status) == SND_PCM_STATE_XRUN)
                {   struct timeval now, diff, tstamp;

                    gettimeofday (&now, 0);
                    snd_pcm_status_get_trigger_tstamp (status, &tstamp);
                    timersub (&now, &tstamp, &diff);

                    fprintf (stderr, "alsa_write_float xrun: of at least %.3f msecs. resetting stream\n",
                             diff.tv_sec * 1000 + diff.tv_usec / 1000.0);
                }
                else
                    fprintf (stderr, "alsa_write_float: xrun. can't determine length\n");
            };
#endif

            snd_pcm_prepare (this->alsa_dev);
            break;

        case -EBADFD:
            fprintf (stderr, "alsa_write: Bad PCM state.n");
            return 0;
            break;

        case -ESTRPIPE:
            fprintf (stderr, "alsa_write: Suspend event.n");
            return 0;
            break;

        case -EIO:
            puts ("alsa_write: EIO");
            return 0;

        default:
            fprintf (stderr, "alsa_write: retval = %d\n", retval);
            return 0;
            break;
        } /* switch */
    } /* while */

    return total;
}

int ALSAOutput::write (int16_t* buffer, unsigned int frames)
{
  return 0;
}



// Accessor methods
//  


// Other methods
//  


void ALSAOutput::start()
{
                int err = snd_pcm_start(this->alsa_dev);
                if (err < 0) {
                       throw runtime_error("unable to start pcm (" + string(snd_strerror(err)) + ")");
      }
}

