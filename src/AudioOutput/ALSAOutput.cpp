#include "ALSAOutput.h"

#include "CommonExceptions.h"
#include "AtomicWrite.h"

#include <iostream>
#include <string>

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <sys/time.h>

#if defined (__linux__)
#include     <fcntl.h>
#include     <sys/ioctl.h>
#include     <sys/soundcard.h>
#endif

ALSAOutput::ALSAOutput ()
{
}

ALSAOutput::~ALSAOutput ()
{
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
            THROW_RUNTIME_ERROR("cannot open audio device \"" << device << "\" (" << snd_strerror(err) << ")");
        }
    }
} /* alsa_open */

void ALSAOutput::init(unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime)
{
    CLOG(LogLevel::DEBUG,"srate:" << sampleRate << " ; channels: " << (int)channels << endl);
    // changing hw settings can only safely be done when pcm is not running
    // therefore stop the pcm and drain all pending frames
    // DO NOT drop pending frames, due to latency some frames might still be played,
    // dropping them will cause (at least in non-realtime mode) a hearable crack

    // UPDATE (2016-03-23): draining may take very long for some reason (>10s !!!) thus try dropping
    this->drop();

    int err;

    // block when calling ALSA API
    snd_pcm_nonblock (this->alsa_dev, 0);

    snd_pcm_hw_params_t* hw_params;
    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
    {
        THROW_RUNTIME_ERROR("cannot allocate hardware parameter structure (" << snd_strerror(err) << ")");
    }

    if ((err = snd_pcm_hw_params_any (this->alsa_dev, hw_params)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot initialize hardware parameter structure (" << snd_strerror(err) << ")");
    }

    if ((err = snd_pcm_hw_params_set_access (this->alsa_dev, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot set access type (" << snd_strerror(err) << ")");
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
    case int32:
        err = snd_pcm_hw_params_set_format (this->alsa_dev, hw_params, SND_PCM_FORMAT_S32);
        break;
    case unknown:
        THROW_RUNTIME_ERROR("ALSAOutput::init(): Sample Format not set");

    default:
        throw NotImplementedException();
        break;
    }
    if (err < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot set sample format (" << snd_strerror(err) << ")");
    }

    if ((err = snd_pcm_hw_params_set_rate_near (this->alsa_dev, hw_params, &sampleRate, 0)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot set sample rate (" << snd_strerror(err) << ")");
    }

    if ((err = snd_pcm_hw_params_set_channels (this->alsa_dev, hw_params, channels)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot set channel count (" << snd_strerror(err) << ")");
    }
// end interesting

    snd_pcm_uframes_t alsa_period_size, alsa_buffer_frames;
    if (realtime)
    {
        alsa_period_size = 256;
        alsa_buffer_frames = 3 * alsa_period_size;
    }
    else
    {
        alsa_period_size = 1024;
        alsa_buffer_frames = 4 * alsa_period_size;
    }

    if ((err = snd_pcm_hw_params_set_buffer_size_near (this->alsa_dev, hw_params, &alsa_buffer_frames)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot set buffer size (" << snd_strerror(err) << ")");
    }

    if ((err = snd_pcm_hw_params_set_period_size_near (this->alsa_dev, hw_params, &alsa_period_size, 0)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot set period size (" << snd_strerror(err) << ")");
    }

    if ((err = snd_pcm_hw_params (this->alsa_dev, hw_params)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot set parameters (" << snd_strerror(err) << ")");
    }


    /* extra check: if we have only one period, this code won't work */
    snd_pcm_hw_params_get_period_size (hw_params, &alsa_period_size, 0);

    snd_pcm_uframes_t buffer_size;
    snd_pcm_hw_params_get_buffer_size (hw_params, &buffer_size);
    if (alsa_period_size == buffer_size)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("Can't use period equal to buffer size (" << alsa_period_size << " == " << buffer_size << ")");
    }

    snd_pcm_hw_params_free (hw_params);


    snd_pcm_sw_params_t *sw_params;
    if ((err = snd_pcm_sw_params_malloc (&sw_params)) != 0)
    {
        THROW_RUNTIME_ERROR("snd_pcm_sw_params_malloc: " << snd_strerror(err));
    }

    if ((err = snd_pcm_sw_params_current (this->alsa_dev, sw_params)) != 0)
    {
        snd_pcm_sw_params_free (sw_params);
        THROW_RUNTIME_ERROR("snd_pcm_sw_params_current: " << snd_strerror(err));
    }

    /* note: set start threshold to delay start until the ring buffer is full */
    if ((err = snd_pcm_sw_params_set_start_threshold (this->alsa_dev, sw_params, buffer_size)) < 0)
    {
        snd_pcm_sw_params_free (sw_params);
        THROW_RUNTIME_ERROR("cannot set start threshold (" << snd_strerror(err) << ")");
    }

    if ((err = snd_pcm_sw_params (this->alsa_dev, sw_params)) != 0)
    {
        snd_pcm_sw_params_free (sw_params);
        THROW_RUNTIME_ERROR("snd_pcm_sw_params: " << snd_strerror(err));
    }

    snd_pcm_sw_params_free (sw_params);

    // WOW, WE MADE IT TIL HERE, so update channelcount, srate and sformat
    this->currentChannelCount = channels;
    this->currentSampleFormat = s;
    this->currentSampleRate   = sampleRate;
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

int ALSAOutput::write (float* buffer, frame_t frames)
{
    return this->write<float>(buffer, frames);
}

int ALSAOutput::write (int16_t* buffer, frame_t frames)
{
    return this->write<int16_t>(buffer, frames);
}

int ALSAOutput::write (int32_t* buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
}

template<typename T> int ALSAOutput::write(T* buffer, frame_t frames)
{
    const int items = frames*this->currentChannelCount;
    T* processedBuffer = new T[items];
    this->getAmplifiedBuffer<T>(buffer, processedBuffer, items);
    buffer = processedBuffer;


    if (this->epipe_count > 0)
    {
        this->epipe_count--;
    }

    int total = 0;
    while (total < frames)
    {
        int retval = snd_pcm_writei(this->alsa_dev, buffer + total * this->currentChannelCount, frames - total);

        if (retval >= 0)
        {
            total += retval;
            if (total == frames)
            {
                return total;
            }

            continue;
        };

        switch (retval)
        {
        case -EAGAIN:
            cout << "alsa_write: EAGAIN" << endl;
            continue;
            break;

        case -EPIPE:
            if (this->epipe_count > 0)
            {
                cout << "alsa_write: EPIPE " << this->epipe_count << endl;
                if (this->epipe_count > 140)
                {
                    return retval;
                }
            };
            this->epipe_count += 100;
            snd_pcm_prepare (this->alsa_dev);
            break;

        case -EBADFD:
            cerr << "alsa_write: Bad PCM state" << endl;
            return 0;
            break;

        case -ESTRPIPE:
            cerr << "alsa_write: Suspend event" << endl;
            return 0;
            break;

        case -EIO:
            cout << "alsa_write: EIO" << endl;
            return 0;

        default:
            cerr << "alsa_write: retval = " << retval << endl;
            return 0;
            break;
        } /* switch */
    } /* while */

    delete [] processedBuffer;
    
    return total;
}

void ALSAOutput::start()
{
//     snd_pcm_reset (this->alsa_dev);
    snd_pcm_prepare(this->alsa_dev);
    int err = snd_pcm_start(this->alsa_dev);
    if (err < 0)
    {
        THROW_RUNTIME_ERROR("unable to start pcm (" << snd_strerror(err) << ")");
    }
}

void ALSAOutput::stop()
{
    this->drop();
}

