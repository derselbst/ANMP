#include "ALSAOutput.h"
#include "IAudioOutput_impl.h"

#include "CommonExceptions.h"
#include "AtomicWrite.h"
#include "Config.h"

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


void ALSAOutput::SetOutputChannels(Nullable<uint16_t> chan)
{
    this->IAudioOutput::SetOutputChannels(chan);
    
    this->_init(this->currentFormat);
}

void ALSAOutput::init(SongFormat* format, bool realtime)
{
    if((this->currentFormat!=nullptr && *this->currentFormat == *format) || !format->IsValid())
    {
        // nothing
    }
    else
    {
        this->_init(format, realtime);
    }

    // made it till here, update channelcount, srate and sformat
    this->currentFormat = format;
}

void ALSAOutput::_init(SongFormat* format, bool realtime)
{
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
    switch(format->SampleFormat)
    {
    case SampleFormat_t::float32:
        err = snd_pcm_hw_params_set_format (this->alsa_dev, hw_params, SND_PCM_FORMAT_FLOAT);
        break;
    case SampleFormat_t::int16:
        err = snd_pcm_hw_params_set_format (this->alsa_dev, hw_params, SND_PCM_FORMAT_S16);
        break;
    case SampleFormat_t::int32:
        err = snd_pcm_hw_params_set_format (this->alsa_dev, hw_params, SND_PCM_FORMAT_S32);
        break;
    case SampleFormat_t::unknown:
        THROW_RUNTIME_ERROR("Sample Format not set");

    default:
        throw NotImplementedException();
        break;
    }
    if (err < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot set sample format (" << snd_strerror(err) << ")");
    }

    if ((err = snd_pcm_hw_params_set_rate_near (this->alsa_dev, hw_params, &format->SampleRate, 0)) < 0)
    {
        snd_pcm_hw_params_free (hw_params);
        THROW_RUNTIME_ERROR("cannot set sample rate (" << snd_strerror(err) << ")");
    }

    if ((err = snd_pcm_hw_params_set_channels (this->alsa_dev, hw_params, this->GetOutputChannels().Value)) < 0)
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
        alsa_period_size = gConfig.FramesToRender;
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

int ALSAOutput::write (const float* buffer, frame_t frames)
{
    return this->write<float>(buffer, frames);
}

int ALSAOutput::write (const int16_t* buffer, frame_t frames)
{
    return this->write<int16_t>(buffer, frames);
}

int ALSAOutput::write (const int32_t* buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
}

template<typename T> int ALSAOutput::write(const T* buffer, frame_t frames)
{
    const int items = frames*this->GetOutputChannels().Value;
    T* processedBuffer = new T[items];
    this->Mix<T, T>(buffer, processedBuffer, frames);
    buffer = processedBuffer;


    if (this->epipe_count > 0)
    {
        this->epipe_count--;
    }

    int total = 0;
    while (total < frames)
    {
        int retval = snd_pcm_writei(this->alsa_dev, buffer + total * this->currentFormat->Channels(), frames - total);

        if (retval >= 0)
        {
            total += retval;
            if (total == frames)
            {
                goto LEAVE_LOOP;
            }

            continue;
        }

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
                    // well actually we wrote even less than nothing, but better return non negative here
                    total = 0;
                    goto LEAVE_LOOP;
                }
            };
            this->epipe_count += 100;
            snd_pcm_prepare (this->alsa_dev);
            break;

        case -EBADFD:
            cerr << "alsa_write: Bad PCM state" << endl;
            total = 0;
            goto LEAVE_LOOP;

        case -ESTRPIPE:
            cerr << "alsa_write: Suspend event" << endl;
            total = 0;
            goto LEAVE_LOOP;

        case -EIO:
            cout << "alsa_write: EIO" << endl;
            total = 0;
            goto LEAVE_LOOP;

        default:
            cerr << "alsa_write: retval = " << retval << endl;
            total = 0;
            goto LEAVE_LOOP;
        } /* switch */
    } /* while */
LEAVE_LOOP:
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
    this->IAudioOutput::stop();
}

