#include "PortAudioOutput.h"
#include "IAudioOutput_impl.h"

#include "CommonExceptions.h"
#include "AtomicWrite.h"
#include "Config.h"

#include <iostream>
#include <string>


PortAudioOutput::PortAudioOutput ()
{
}

PortAudioOutput::~PortAudioOutput ()
{
    this->close();

    if (this->paInitError == PaErrorCode::paNoError)
    {
        Pa_Terminate();
    }
}

//
// Interface Implementation
//
void PortAudioOutput::open()
{
    if(this->handle == nullptr)
    {
        this->paInitError = Pa_Initialize();
        if (this->paInitError != PaErrorCode::paNoError)
        {
            THROW_RUNTIME_ERROR("unable to initialize portaudio (" << Pa_GetErrorText(this->paInitError) << ")");
        }
    }
}


void PortAudioOutput::SetOutputChannels(Nullable<uint16_t> chan)
{
    this->IAudioOutput::SetOutputChannels(chan);
    
    // force reinit
    if(this->currentFormat.IsValid() && chan.hasValue)
    {
        this->_init(this->currentFormat);
    }
}

void PortAudioOutput::init(SongFormat& format, bool realtime)
{
    if(format.IsValid())
    {
        if(this->currentFormat.SampleFormat == format.SampleFormat && this->currentFormat.SampleRate == format.SampleRate)
        {
            // nothing
        }
        else
        {
            this->_init(format, realtime);
        }
    }
    
    // finally update channelcount, srate and sformat
    this->currentFormat = format;
}

void PortAudioOutput::_init(SongFormat& format, bool realtime)
{
    if (this->paInitError != PaErrorCode::paNoError)
    {
        THROW_RUNTIME_ERROR("portaudio not initialized! (" << Pa_GetErrorText(this->paInitError) << ")");
    }

    PaSampleFormat paSampleFmt;

    switch(format.SampleFormat)
    {
    case SampleFormat_t::float32:
        paSampleFmt = paFloat32;
        break;
    case SampleFormat_t::int16:
        paSampleFmt =  paInt16;
        break;
    case SampleFormat_t::int32:
        paSampleFmt =  paInt32;
        break;
    case SampleFormat_t::unknown:
        THROW_RUNTIME_ERROR("Sample Format not set");

    default:
        throw NotImplementedException();
        break;
    }

    this->drop();
    this->close();

    PaError err;

    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream( &this->handle,
                                0,          /* no input channels */
                                this->GetOutputChannels().Value,   /* no. of output channels */
                                paSampleFmt,  /* 32 bit floating point output */
                                format.SampleRate,
                                gConfig.FramesToRender,  /* frames per buffer, i.e. the number of sample frames that PortAudio will request from the callback.*/
                                nullptr, /* this is my callback function */
                                this ); /* This is a pointer that will be passed to my callback */

    if (err != PaErrorCode::paNoError)
    {
        THROW_RUNTIME_ERROR("unable to stop pcm (" << Pa_GetErrorText(err) << ")");
    }
}

void PortAudioOutput::drain()
{
    this->stop();
}

void PortAudioOutput::drop()
{
    this->stop();
}

void PortAudioOutput::close()
{
    if (this->handle != nullptr)
    {
        Pa_CloseStream( this->handle );
        this->handle = nullptr;
    }
}

int PortAudioOutput::write (const float* buffer, frame_t frames)
{
    return this->write<float>(buffer, frames);
}

int PortAudioOutput::write (const int16_t* buffer, frame_t frames)
{
    return this->write<int16_t>(buffer, frames);
}

int PortAudioOutput::write (const int32_t* buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
}

template<typename T> int PortAudioOutput::write(const T* buffer, frame_t frames)
{
    if(this->handle == nullptr)
    {
        THROW_RUNTIME_ERROR("unable to write pcm since PortAudioOutput::init() has not been called yet or init failed");
    }

    const uint16_t Channels = this->GetOutputChannels().Value;
    T* processedBuffer = new T[frames * Channels];
    this->Mix<T, T>(frames, buffer, this->currentFormat, processedBuffer, Channels);

    Pa_WriteStream(this->handle, processedBuffer, frames);

    delete [] processedBuffer;

    // well, just hope that all of them have actually been written
    return frames;
}

void PortAudioOutput::start()
{
    if(this->handle != nullptr)
    {
        PaError err = Pa_StartStream( this->handle );
        if (err != PaErrorCode::paNoError && err != paStreamIsNotStopped)
        {
            THROW_RUNTIME_ERROR("unable to start pcm (" << Pa_GetErrorText(err) << ")");
        }
    }
    else
    {
        THROW_RUNTIME_ERROR("unable to start pcm since PortAudioOutput::init() has not been called yet or init failed");
    }
}

void PortAudioOutput::stop()
{
    if(this->handle != nullptr)
    {
        // dont call Pa_StopStream() here since it causes draining the pcm, which takes time and may cause deadlocks
        // use Pa_AbortStream() instead which drops any PCM currently played
        PaError err = Pa_AbortStream( this->handle );
        if(err != PaErrorCode::paNoError && err != PaErrorCode::paStreamIsStopped)
        {
            THROW_RUNTIME_ERROR("unable to stop pcm (" << Pa_GetErrorText(err) << ")");
        }
    }
}

