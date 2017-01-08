#include "PortAudioOutput.h"

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

void PortAudioOutput::init(SongFormat format, bool realtime)
{
    if(this->currentFormat == format || !format.IsValid())
    {
        return;
    }
        
    if (this->paInitError != PaErrorCode::paNoError)
    {
        THROW_RUNTIME_ERROR("portaudio not initialized! (" << Pa_GetErrorText(this->paInitError) << ")");
    }

    PaSampleFormat paSampleFmt;

    switch(format.SampleFormat)
    {
    case float32:
        paSampleFmt = paFloat32;
        break;
    case int16:
        paSampleFmt =  paInt16;
        break;
    case int32:
        paSampleFmt =  paInt32;
        break;
    case unknown:
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
                                format.Channels,   /* no. of output channels */
                                paSampleFmt,  /* 32 bit floating point output */
                                format.SampleRate,
                                gConfig.FramesToRender,  /* frames per buffer, i.e. the number of sample frames that PortAudio will request from the callback.*/
                                nullptr, /* this is my callback function */
                                this ); /* This is a pointer that will be passed to my callback */

    if (err != PaErrorCode::paNoError)
    {
        THROW_RUNTIME_ERROR("unable to stop pcm (" << Pa_GetErrorText(err) << ")");
    }

    // finally update channelcount, srate and sformat
    this->currentFormat = format;
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

    const int items = frames*this->currentFormat.Channels;
    T* processedBuffer = new T[items];
    this->getAmplifiedBuffer(buffer, processedBuffer, items);
    buffer = processedBuffer;

    Pa_WriteStream(this->handle, buffer, frames );

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

