#include "JackOutput.h"

#include "CommonExceptions.h"
#include "AtomicWrite.h"

#include <iostream>
#include <string>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <cstring>

#include <jack/jack.h>


JackOutput::JackOutput ()
{
    this->srcData.data_in = nullptr;
    this->srcData.data_out = nullptr;
    this->srcData.input_frames = 0;
    this->srcData.output_frames = 0;
}

JackOutput::~JackOutput ()
{
    this->close();

    if(this->srcState != nullptr)
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
    if(this->handle == nullptr)
    {

        jack_status_t status;
        jack_options_t options = JackNullOption;

        this->handle = jack_client_open (JackOutput::ClientName, options, &status, nullptr);

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
    }
}

void JackOutput::init(SongFormat format, bool realtime)
{
    if(!format.IsValid())
    {
        return;
    }
    
    // shortcut
    decltype(format.Channels)& channels = format.Channels;
    
    // avoid jack thread Interference
    lock_guard<recursive_mutex> lck(this->mtx);
    
    // channel count differ?
    if(this->currentFormat.Channels != channels)
    {
        // channel count changed, buffer need to be realloced
        JackOutput::onJackBufSizeChanged(jack_get_buffer_size(this->handle), this);
        
        // register "channels" number of input ports
        if(this->playbackPorts.size() < channels)
        {
            char portName[3+1];
            for(unsigned int i=this->playbackPorts.size(); i<channels && i <= 99; i++)
            {
                snprintf(portName, 3+1, "%.2d", i);

                // we provide an output port (from the view of jack), i.e. a capture port, i.e. a readable port
                jack_port_t* out_port = jack_port_register (this->handle, portName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

                if (out_port == nullptr)
                {
                    // TODO: throw or just break??
                    THROW_RUNTIME_ERROR("no more JACK ports available");
                }

                this->playbackPorts.push_back(out_port);
            }
            
            this->connectPorts();
        }
        else if(this->playbackPorts.size() > channels)
        {
            // 2016-08-06: dont deregister port!
            /*          // assumption that (this->currentFormat.Channels == this->playbackPorts.size()) may not always be correct
                for(int i=this->playbackPorts.size()-1; i>=channels; i--)
                {
                    int err = jack_port_unregister (this->handle, this->playbackPorts[i]);
                    if (err != 0)
                    {
                        // ?? what to do ??
                    }

                    this->playbackPorts.pop_back();
                }*/
        }
        else
        {
            // this->playbackPorts.size() == channels, nothing to do here
        }
        
        // we have to delete the resampler in order to refresh channel count
        if(this->srcState != nullptr)
        {
            this->srcState = src_delete(this->srcState);
        }
        int error;
        // SRC_SINC_BEST_QUALITY is too slow, causing jack process thread to discard samples, resulting in hearable artifacts
        // SRC_LINEAR has high frequency audible garbage
        // SRC_ZERO_ORDER_HOLD is even worse than LINEAR
        // SRC_SINC_MEDIUM_QUALITY might still be too slow when using jack with very low latency having a bit of CPU load
        this->srcState = src_new(SRC_SINC_FASTEST, channels, &error);
        if(this->srcState == nullptr)
        {
            THROW_RUNTIME_ERROR("unable to init libsamplerate (" << src_strerror(error) <<")");
        }
    }
    else
    {
        // zero out any buffer in resampler, to avoid hearable cracks, when switching from one song to another
        src_reset(this->srcState);
    }
    
    // WOW, WE MADE IT TIL HERE, so update channelcount, srate and sformat
    this->currentFormat = format;
}

void JackOutput::close()
{
    if (this->handle != nullptr)
    {
        jack_client_close (this->handle);
        this->handle = nullptr;
    }
}

int JackOutput::write (const float* buffer, frame_t frames)
{
    lock_guard<recursive_mutex> lock(this->mtx);
    
    if(this->interleavedProcessedBuffer.ready)
    {
        // buffer has not been consumed yet
        return 0;
    }

    const size_t Items = frames*this->currentFormat.Channels;

    // converted_to_float_but_not_resampled buffer
    float* tempBuf = new float[Items];
    this->getAmplifiedBuffer<float>(buffer, tempBuf, Items);

    int framesUsed = this->doResampling(tempBuf, frames);

    delete[] tempBuf;

    return framesUsed;
}

template<typename T> void JackOutput::getAmplifiedFloatBuffer(const T* inBuf, float* outBuf, const size_t Items)
{
    for(unsigned int i = 0; i<Items; i++)
    {
        float& item = outBuf[i];
        // convert that item in inBuf[i] to float
        item = inBuf[i] / (static_cast<float>(numeric_limits<T>::max())+1);
        // amplify
        item *= this->volume;

        // clip
        if(item > 1.0f)
        {
            item = 1.0f;
        }
        else if(item < -1.0f)
        {
            item = -1.0f;
        }
    }
}

int JackOutput::doResampling(const float* inBuf, const size_t Frames)
{
    lock_guard<recursive_mutex> lock(this->mtx);
    
    this->srcData.data_in = /*const_cast<float*>*/(inBuf);
    this->srcData.input_frames = Frames;

    // this is not the last buffer passed to src
    this->srcData.end_of_input = 0;

    this->srcData.data_out = this->interleavedProcessedBuffer.buf;
    this->srcData.data_out += this->srcData.output_frames_gen * this->currentFormat.Channels;

    this->srcData.output_frames = this->jackBufSize - this->srcData.output_frames_gen;

    // just to be sure
    this->srcData.output_frames_gen = 0;

    // output_sample_rate / input_sample_rate
    this->srcData.src_ratio = (double)this->jackSampleRate / this->currentFormat.SampleRate;

    int err = src_process (this->srcState, &this->srcData);
    if(err != 0 )
    {
        CLOG(LogLevel_t::Error, "libsamplerate failed processing (" << src_strerror(err) <<")");
    }
    else
    {
        if(this->srcData.output_frames_gen < this->srcData.output_frames)
        {
            CLOG(LogLevel_t::Warning, "jacks callback buffer has not been filled completely" << endl <<
                    "input_frames: " << this->srcData.input_frames << "\toutput_frames: " << this->srcData.output_frames << endl <<
                    "input_frames_used: " << this->srcData.input_frames_used << "\toutput_frames_gen: " << this->srcData.output_frames_gen);

// 				const size_t diffItems = this->srcData.output_frames - this->srcData.output_frames_gen;
// 				memset(this->interleavedProcessedBuffer.buf, 0, diffItems * sizeof(jack_default_audio_sample_t));
        }
        else
        {
            this->srcData.output_frames_gen = 0;
            this->interleavedProcessedBuffer.ready = true;
        }
    }
    return this->srcData.input_frames_used;
}

template<typename T> int JackOutput::write (const T* buffer, frame_t frames)
{
    lock_guard<recursive_mutex> lock(this->mtx);
    
    if(this->interleavedProcessedBuffer.ready)
    {
        // buffer has not been consumed yet
        return 0;
    }

    const size_t Items = frames*this->currentFormat.Channels;

    // converted_to_float_but_not_resampled buffer
    float* tempBuf = new float[Items];
    this->getAmplifiedFloatBuffer<T>(buffer, tempBuf, Items);

    int framesUsed = this->doResampling(tempBuf, frames);

    delete[] tempBuf;
    return framesUsed;

}

int JackOutput::write (const int16_t* buffer, frame_t frames)
{
    return this->write<int16_t>(buffer, frames);
}

int JackOutput::write (const int32_t* buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
}


void JackOutput::connectPorts()
{
    const char** physicalPlaybackPorts = jack_get_ports(this->handle, NULL, NULL, JackPortIsPhysical|JackPortIsInput);

    if(physicalPlaybackPorts==nullptr)
    {
        CLOG(LogLevel_t::Error, "no physical playback ports available");
        return;
    }

    for(unsigned int i=0; physicalPlaybackPorts[i]!=nullptr && i<this->playbackPorts.size(); i++)
    {
        if (jack_connect(this->handle, jack_port_name(this->playbackPorts[i]), physicalPlaybackPorts[i]))
        {
            CLOG(LogLevel_t::Info, "cannot connect to port \"" << physicalPlaybackPorts[i] << "\"");
        }
    }

    jack_free(const_cast<char**>(physicalPlaybackPorts));
}

void JackOutput::start()
{
    if (jack_activate(this->handle))
    {
        THROW_RUNTIME_ERROR("cannot activate client")
    }
    
    lock_guard<recursive_mutex> lck(this->mtx);
    
    // avoid playing any outdated garbage
    this->interleavedProcessedBuffer.ready = false;
    
    this->interleavedProcessedBuffer.isRunning = true;
    
    // zero out any buffer in resampler, to avoid hearable cracks, when pausing and restarting playback
    src_reset(this->srcState);
    
    this->connectPorts();
}

void JackOutput::stop()
{
// we should deactivate the client here, but if we do, any other jack client recording our output will get in trouble
//     jack_deactivate(this->handle);
    
    this->interleavedProcessedBuffer.isRunning = false;
}

int JackOutput::processCallback(jack_nframes_t nframes, void* arg)
{
    JackOutput *const pthis = static_cast<JackOutput*const>(arg);

    
    if(!pthis->mtx.try_lock())
    {
        goto fail;
    }
    
    if(!pthis->interleavedProcessedBuffer.isRunning)
    {
    pthis->mtx.unlock();
        goto fail;
    }
    
    if (!pthis->interleavedProcessedBuffer.ready)
    {
    pthis->mtx.unlock();
        CLOG(LogLevel_t::Warning, "buffer was not ready for jack, discarding");
        goto fail;
    }
    
    if(nframes != pthis->jackBufSize)
    {
    pthis->mtx.unlock();
        CLOG(LogLevel_t::Error, "expected JackBufSize of " << pthis->jackBufSize << ", got " << nframes);
        goto fail;
    }

    for(unsigned int i=0; i<pthis->playbackPorts.size(); i++)
    {
        jack_default_audio_sample_t* out = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(pthis->playbackPorts[i], nframes));
        
        // there might be more ports than currently channels
        if(i<pthis->currentFormat.Channels)
        {
            for(unsigned int myIdx=i, jackIdx=0; jackIdx<nframes; myIdx+=pthis->currentFormat.Channels, jackIdx++)
            {
                out[jackIdx] = pthis->interleavedProcessedBuffer.buf[myIdx];
                pthis->interleavedProcessedBuffer.buf[myIdx] = 0; // zero the consumed buffer
            }       
        }
        else
        {
            // mute that port
            memset(out, 0, nframes*sizeof(jack_default_audio_sample_t));
        }
    }
    pthis->interleavedProcessedBuffer.ready = false;

    pthis->mtx.unlock();
    return 0;

fail:
    for(unsigned int i=0; i<pthis->playbackPorts.size(); i++)
    {
        jack_default_audio_sample_t* out = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(pthis->playbackPorts[i], nframes));

        memset(out, 0, nframes*sizeof(jack_default_audio_sample_t));
    }
    return 0;
}

// we can do non-realtime safe stuff here :D
int JackOutput::onJackBufSizeChanged(jack_nframes_t nframes, void *arg)
{
    JackOutput *const pthis = static_cast<JackOutput*const>(arg);
    lock_guard<recursive_mutex> lock(pthis->mtx);

    pthis->jackBufSize = nframes;

    // if buffer not consumed, print a warning and discard samples, who cares...
    if(pthis->interleavedProcessedBuffer.ready)
    {
        CLOG(LogLevel_t::Warning, "JackBufSize changed, but buffer has not been consumed yet, discarding pending samples");
    }

    delete[] pthis->interleavedProcessedBuffer.buf;
    pthis->interleavedProcessedBuffer.buf = new (nothrow) jack_default_audio_sample_t[pthis->jackBufSize * pthis->currentFormat.Channels];
    pthis->interleavedProcessedBuffer.ready = false;

    // TODO check if alloc successfull

    return 0;
}


int JackOutput::onJackSampleRateChanged(jack_nframes_t nframes, void* arg)
{
    JackOutput *const pthis = static_cast<JackOutput*const>(arg);

    pthis->jackSampleRate = nframes;

    return 0;
}

void JackOutput::onJackShutdown(void* arg)
{
}
