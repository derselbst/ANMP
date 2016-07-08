#include "PortAudioOutput.h"

#include "CommonExceptions.h"
#include "AtomicWrite.h"

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
	if(this->handle != nullptr)
	{
		this->paInitError = Pa_Initialize();
		if (this->paInitError != PaErrorCode::paNoError)
		{
			THROW_RUNTIME_ERROR("unable to initialize portaudio (" << Pa_GetErrorText(this->paInitError) << ")");
		}
	}
}

void PortAudioOutput::init(unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime)
{
	if (this->paInitError != PaErrorCode::paNoError)
	{
		THROW_RUNTIME_ERROR("portaudio not initialized! (" << Pa_GetErrorText(this->paInitError) << ")");
	}
	
	PaSampleFormat paSampleFmt;
	
    switch(s)
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
                                channels,   /* no. of output channels */
                                paSampleFmt,  /* 32 bit floating point output */
                                sampleRate,
                                Config::FramesToRender,  /* frames per buffer, i.e. the number of sample frames that PortAudio will request from the callback.*/
                                nullptr, /* this is my callback function */
                                this ); /* This is a pointer that will be passed to my callback */
	
	
	

    // finally update channelcount, srate and sformat
    this->currentChannelCount = channels;
    this->currentSampleFormat = s;
    this->currentSampleRate   = sampleRate;
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

int PortAudioOutput::write (float* buffer, frame_t frames)
{
    return this->write<float>(buffer, frames);
}

int PortAudioOutput::write (int16_t* buffer, frame_t frames)
{
    return this->write<int16_t>(buffer, frames);
}

int PortAudioOutput::write (int32_t* buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
}

template<typename T> int PortAudioOutput::write(T* buffer, frame_t frames)
{   
	 if(this->handle == nullptr)
	{
		THROW_RUNTIME_ERROR("unable to write pcm since PortAudioOutput::init() has not been called yet or init failed");
	}
	
    const int items = frames*this->currentChannelCount;
    T processedBuffer[items];
    this->getAmplifiedBuffer(buffer, processedBuffer, items);
    buffer = processedBuffer;
    
    Pa_WriteStream( this->handle, buffer, frames );
}

void PortAudioOutput::start()
{
	if(this->handle != nullptr)
	{
		PaError err = Pa_StartStream( this->handle );
		if (err != PaErrorCode::paNoError)
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
		PaError err = Pa_StopStream( this->handle );
		if (err != PaErrorCode::paNoError)
		{
			THROW_RUNTIME_ERROR("unable to stop pcm (" << Pa_GetErrorText(err) << ")");
		}
	}
}
