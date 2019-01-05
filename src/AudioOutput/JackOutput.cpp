#include "JackOutput.h"
#include "Config.h"
#include "IAudioOutput_impl.h"

#include "AtomicWrite.h"
#include "CommonExceptions.h"

#include <chrono> // std::chrono::seconds
#include <cstring>
#include <iostream>
#include <string>
#include <thread> // std::this_thread::sleep_for

#include <jack/jack.h>


JackOutput::JackOutput()
{
    memset(&this->srcData, 0, sizeof(this->srcData));
}

JackOutput::~JackOutput()
{
    this->close();

    if (this->srcState != nullptr)
    {
        this->srcState = src_delete(this->srcState);
    }

    delete[] this->interleavedProcessedBuffer.buf;
    this->interleavedProcessedBuffer.buf = nullptr;
}

//
// Interface Implementation
//
void JackOutput::open()
{
    if (this->handle == nullptr)
    {
        jack_status_t status;
        jack_options_t options = JackNullOption;

        this->handle = jack_client_open(JackOutput::ClientName, options, &status, nullptr);

        if (this->handle == nullptr)
        {
            if (status & JackServerFailed)
            {
                THROW_RUNTIME_ERROR("Unable to connect to JACK server");
            }
            else
            {
                THROW_RUNTIME_ERROR("jack_client_open() failed, status: " << status);
            }
        }

        if (status & JackServerStarted)
        {
            CLOG(LogLevel_t::Info, "JACK server started");
        }
        if (status & JackNameNotUnique)
        {
            this->ClientName = jack_get_client_name(this->handle);
            CLOG(LogLevel_t::Warning, "unique name " << this->ClientName << " assigned");
        }

        jack_set_process_callback(this->handle, JackOutput::processCallback, this);
        jack_set_sample_rate_callback(this->handle, JackOutput::onJackSampleRateChanged, this);
        jack_on_shutdown(this->handle, JackOutput::onJackShutdown, this);
        jack_set_buffer_size_callback(this->handle, JackOutput::onJackBufSizeChanged, this);

        // initially assign the sample rate
        JackOutput::onJackSampleRateChanged(jack_get_sample_rate(this->handle), this);

        // initially create jack ports
        this->JackOutput::SetOutputChannels(2);
    }
}

void JackOutput::SetOutputChannels(Nullable<uint16_t> chan)
{
    // deregister all ports
    for (int i = this->playbackPorts.size() - 1; i >= 0; i--)
    {
        int err = jack_port_unregister(this->handle, this->playbackPorts[i]);
        if (err != 0)
        {
            // ?? what to do ??
        }

        this->playbackPorts.pop_back();
    }

    if (chan.hasValue)
    {
        this->tempBuf.clear();
        this->tempBuf.shrink_to_fit();
        this->tempBuf.resize(gConfig.FramesToRender * chan.Value);
        
        // re-register ports
        char portName[3 + 1];
        for (unsigned int i = this->playbackPorts.size(); i < chan.Value && i <= 99; i++)
        {
            snprintf(portName, sizeof(portName), "%.2d", i);

            // we provide an output port (from the view of jack), i.e. a capture port, i.e. a readable port
            jack_port_t *out_port = jack_port_register(this->handle, portName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

            if (out_port == nullptr)
            {
                THROW_RUNTIME_ERROR("no more JACK ports available");
            }

            this->playbackPorts.push_back(out_port);
        }

        // we have to delete the resampler in order to refresh channel count
        if (this->srcState != nullptr)
        {
            this->srcState = src_delete(this->srcState);
        }
        int error;
        // SRC_SINC_BEST_QUALITY is too slow, causing jack process thread to discard samples, resulting in hearable artifacts
        // SRC_LINEAR has high frequency audible garbage
        // SRC_ZERO_ORDER_HOLD is even worse than LINEAR
        // SRC_SINC_MEDIUM_QUALITY might still be too slow when using jack with very low latency having a bit of CPU load
        this->srcState = src_new(SRC_SINC_FASTEST, chan.Value, &error);
        if (this->srcState == nullptr)
        {
            THROW_RUNTIME_ERROR("unable to init libsamplerate (" << src_strerror(error) << ")");
        }

        // channel count changed, buffer needs to be realloced
        if (JackOutput::onJackBufSizeChanged(jack_get_buffer_size(this->handle), this) != 0)
        {
            // invalidate the current format
            this->currentFormat.SetVoices(0);
            THROW_RUNTIME_ERROR("unable to onJackBufSizeChanged()");
        }

        // start the process thread, no error if it is already started
        if (jack_activate(this->handle))
        {
            THROW_RUNTIME_ERROR("cannot activate client")
        }

        this->connectPorts();
    }

    // call base method to make sure we allocate temporary mixdown buffer
    this->IAudioOutput::SetOutputChannels(min<uint16_t>(chan.Value, this->playbackPorts.size()));
}

void JackOutput::init(SongFormat &format, bool realtime)
{
    if (format.IsValid())
    {
        // only care about samplerates here, reset resampler to avoid "smooth transition"
        // sampleformat is not needed here, we always push floats to jack
        // no. of channels is interesting for IAudioOutput::Mix()
        if (this->currentFormat.SampleRate != format.SampleRate)
        {
            // avoid jack thread Interference
            lock_guard<mutex> lck(this->mtx);

            // zero out any buffer in resampler, to avoid hearable cracks, when switching from one song to another
            src_reset(this->srcState);
        }
    }

    // dont forget to update channelcount, srate and sformat
    this->currentFormat = format;
}

void JackOutput::close()
{
    if (this->handle != nullptr)
    {
        jack_client_close(this->handle);
        this->handle = nullptr;
    }
}


int JackOutput::doResampling(const float *inBuf, const size_t Frames)
{
    if (!this->interleavedProcessedBuffer.isRunning)
    {
        return 0;
    }
    if (!this->currentFormat.IsValid())
    {
        THROW_RUNTIME_ERROR("SongFormat not valid")
    }

    this->srcData.data_in = inBuf;
    this->srcData.input_frames = Frames;

    this->srcData.data_out = this->interleavedProcessedBuffer.buf;
    this->srcData.data_out += this->srcData.output_frames_gen * this->GetOutputChannels().Value;

    this->srcData.output_frames = this->jackBufSize - this->srcData.output_frames_gen;

    // remember the count of frames the already have been resampled
    int old = this->srcData.output_frames_gen;

    // output_sample_rate / input_sample_rate
    this->srcData.src_ratio = (double)this->jackSampleRate / this->currentFormat.SampleRate;

    int err = src_process(this->srcState, &this->srcData);
    if (err != 0)
    {
        CLOG(LogLevel_t::Error, "libsamplerate failed processing (" << src_strerror(err) << ")");
    }
    else
    {
        if (this->srcData.output_frames_gen < this->srcData.output_frames)
        {
            CLOG(LogLevel_t::Info, "jacks callback buffer has not been filled completely" << endl
                                                                                          << "input_frames: " << this->srcData.input_frames << "\toutput_frames: " << this->srcData.output_frames << endl
                                                                                          << "input_frames_used: " << this->srcData.input_frames_used << "\toutput_frames_gen: " << this->srcData.output_frames_gen);

            // needed next time to advance data_out
            this->srcData.output_frames_gen += old;
        }
        else
        {
            this->srcData.output_frames_gen = 0;
            this->interleavedProcessedBuffer.ready = true;
            // notify jacks process thread. in fact doesnt do anything, because jacks process thread doesnt this->cv.wait()
            this->cv.notify_one();
        }
    }
    return this->srcData.input_frames_used;
}

template<typename T>
int JackOutput::write(const T *buffer, frame_t frames)
{
    const uint16_t Channels = this->GetOutputChannels().Value;

    this->Mix<T, float>(frames, buffer, this->currentFormat, this->tempBuf.data(), Channels);

    unique_lock<mutex> lck(this->mtx);

    int framesUsed = 0;
    int framesUsedNow;
    do
    {
        // wait until jacks buffer has been consumed
        this->cv.wait(lck, [this] { return !this->interleavedProcessedBuffer.ready || !this->interleavedProcessedBuffer.isRunning; });

        framesUsedNow = this->doResampling(this->tempBuf.data() + framesUsed * Channels, frames);
        frames -= framesUsedNow;
        framesUsed += framesUsedNow;

    } while (frames > 0 && this->interleavedProcessedBuffer.isRunning);

    lck.unlock();

    return framesUsed;
}

int JackOutput::write(const int16_t *buffer, frame_t frames)
{
    return this->write<int16_t>(buffer, frames);
}

int JackOutput::write(const int32_t *buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
}

int JackOutput::write(const float *buffer, frame_t frames)
{
    return this->write<float>(buffer, frames);
}


void JackOutput::connectPorts()
{
    const char **physicalPlaybackPorts = jack_get_ports(this->handle, NULL, NULL, JackPortIsPhysical | JackPortIsInput);

    if (physicalPlaybackPorts == nullptr)
    {
        CLOG(LogLevel_t::Error, "no physical playback ports available");
        return;
    }

    for (unsigned int i = 0; physicalPlaybackPorts[i] != nullptr && i < this->playbackPorts.size(); i++)
    {
        if (!jack_port_connected_to(this->playbackPorts[i], physicalPlaybackPorts[i]))
        {
            if (jack_connect(this->handle, jack_port_name(this->playbackPorts[i]), physicalPlaybackPorts[i]))
            {
                CLOG(LogLevel_t::Info, "cannot connect to port \"" << physicalPlaybackPorts[i] << "\"");
            }
        }
    }

    jack_free(const_cast<char **>(physicalPlaybackPorts));
}

void JackOutput::start()
{
    {
        lock_guard<mutex> lck(this->mtx);

        // zero out any buffer in resampler, to avoid hearable cracks, when pausing and restarting playback
        src_reset(this->srcState);

        this->interleavedProcessedBuffer.isRunning = true;

        // avoid playing any outdated garbage
        this->interleavedProcessedBuffer.ready = false;
    }

    // noone should be waiting within this->write(), just to be sure
    this->cv.notify_all();
}

void JackOutput::stop()
{
    // we should deactivate the client here, but if we do, any other jack client recording our output will get in trouble
    //     jack_deactivate(this->handle);

    unique_lock<mutex> lck(this->mtx);
    this->interleavedProcessedBuffer.isRunning = false;
    lck.unlock();
    this->cv.notify_all();
}

int JackOutput::processCallback(jack_nframes_t nframes, void *arg)
{
    JackOutput *const pthis = static_cast<JackOutput *const>(arg);
    const unsigned int nJackPorts = pthis->playbackPorts.size();

    // doc says: "return 0 on success, nonzero otherwise"
    // however if we return non zero, jack just silently deactivates us, so we have no other chance than always return 0 here
    int ret = 0;

    if (!pthis->interleavedProcessedBuffer.isRunning)
    {
        //         ret = 0; // no error
        goto fail;
    }

    if (!pthis->interleavedProcessedBuffer.ready)
    {
        CLOG(LogLevel_t::Warning, "buffer was not ready for jack, discarding");

        //         ret = -1;
        goto fail;
    }

    if (nframes != pthis->jackBufSize)
    {
        CLOG(LogLevel_t::Error, "expected JackBufSize of " << pthis->jackBufSize << ", got " << nframes);

        //         ret = -1;
        goto fail;
    }

    if (!pthis->mtx.try_lock())
    {
        CLOG(LogLevel_t::Warning, "acquiring lock failed, discarding");
        //         ret = -1;
        goto fail;
    }

    {
        const unsigned int nchannels = pthis->GetOutputChannels().Value;
        const unsigned int portsToFill = min<unsigned int>(nJackPorts, nchannels);

        jack_default_audio_sample_t *out[nJackPorts]; // temporary array that caches the retrieved buffers for jack ports

        // cache the addresses for jack's ports
        for (unsigned int i = 0; i < nJackPorts; i++)
        {
            out[i] = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(pthis->playbackPorts[i], nframes));
        }

        // run over our downmixed, processed, interleaved pcm buffer frame by frame
        for (unsigned int f = 0; f < nframes; f++)
        {
            // and map each item to its respective jack port
            for (unsigned int c = 0; c < nchannels; c++)
            {
                const unsigned int idx = f * nchannels + c;
                const jack_default_audio_sample_t &item = pthis->interleavedProcessedBuffer.buf[idx];
                out[c][f] = item;
//                 item = 0; // zero the consumed buffer
            }
        }

        // there might be more ports than currently channels
        const int portsLeft = nJackPorts - portsToFill;
        for (int i = 0; i < portsLeft; i++)
        {
            const int idx = i + portsToFill;
            // mute those ports
            memset(out[idx], 0, nframes * sizeof(**out));
        }
    }
    pthis->interleavedProcessedBuffer.ready = false;
    pthis->mtx.unlock();
    pthis->cv.notify_all();

    return ret;

fail:
    for (unsigned int i = 0; i < nJackPorts; i++)
    {
        jack_default_audio_sample_t *out = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(pthis->playbackPorts[i], nframes));

        memset(out, 0, nframes * sizeof(*out));
    }
    return ret;
}

// we can do non-realtime safe stuff here
int JackOutput::onJackBufSizeChanged(jack_nframes_t nframes, void *arg)
{
    JackOutput *const pthis = static_cast<JackOutput *const>(arg);
    unique_lock<mutex> lck(pthis->mtx);

    pthis->jackBufSize = nframes;

    // if buffer not consumed, print a warning and discard samples, who cares...
    if (pthis->interleavedProcessedBuffer.ready)
    {
        CLOG(LogLevel_t::Warning, "JackBufSize changed, but buffer has not been consumed yet, discarding pending samples");
    }

    delete[] pthis->interleavedProcessedBuffer.buf;
    pthis->interleavedProcessedBuffer.buf = new (std::nothrow) jack_default_audio_sample_t[pthis->jackBufSize * pthis->GetOutputChannels().Value];
    pthis->interleavedProcessedBuffer.ready = false;

    if (pthis->interleavedProcessedBuffer.buf == nullptr)
    {
        CLOG(LogLevel_t::Error, "allocating new buffer failed, out of memory");
        pthis->interleavedProcessedBuffer.isRunning = false;
        return -1;
    }

    lck.unlock();
    pthis->cv.notify_all();

    return 0;
}


int JackOutput::onJackSampleRateChanged(jack_nframes_t nframes, void *arg)
{
    JackOutput *const pthis = static_cast<JackOutput *const>(arg);

    pthis->jackSampleRate = nframes;

    return 0;
}

void JackOutput::onJackShutdown(void *)
{
}
