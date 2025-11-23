#include "PortAudioOutput.h"
#include "IAudioOutput_impl.h"

#include "AtomicWrite.h"
#include "CommonExceptions.h"
#include "Config.h"

#include <iostream>
#include <string>
#include <portaudio.h>


struct PortAudioOutput::Impl
{    
    PaStream *handle = nullptr;
    PaDeviceIndex deviceIndex;
    const PaDeviceInfo *deviceInfo;
};

PortAudioOutput::PortAudioOutput() : d(std::make_unique<Impl>())
{
    auto paInitError = Pa_Initialize();
    if (paInitError != PaErrorCode::paNoError)
    {
        THROW_RUNTIME_ERROR("unable to initialize portaudio (" << Pa_GetErrorText(paInitError) << ")");
    }
#ifdef _WIN32
    // Find WASAPI host API index
    int wasapiIndex = -1;
    int hostApiCount = Pa_GetHostApiCount();
    for (int i = 0; i < hostApiCount; ++i)
    {
        const PaHostApiInfo *hai = Pa_GetHostApiInfo(i);
        if (hai && hai->type == paWASAPI)
        {
            wasapiIndex = i;
            break;
        }
    }
    if (wasapiIndex < 0)
    {
        THROW_RUNTIME_ERROR("WASAPI host API not found");
    }

    const PaHostApiInfo *wasapiInfo = Pa_GetHostApiInfo(wasapiIndex);
    d->deviceIndex = wasapiInfo->defaultOutputDevice;
#else
    d->deviceIndex = Pa_GetDefaultOutputDevice();
#endif
    d->deviceInfo = Pa_GetDeviceInfo(d->deviceIndex);
    if (!d->deviceInfo || d->deviceIndex == paNoDevice)
    {
        THROW_RUNTIME_ERROR("Failed to get default WASAPI output device info.");
    }

    CLOG(LogLevel_t::Info, "Using device: " << d->deviceInfo->name);
    CLOG(LogLevel_t::Info, "Max output channels: " << d->deviceInfo->maxOutputChannels);
}

PortAudioOutput::~PortAudioOutput()
{
    this->close();
    Pa_Terminate();
}

//
// Interface Implementation
//
void PortAudioOutput::open()
{
    if (d->handle == nullptr)
    {
    }
}


void PortAudioOutput::SetOutputChannels(uint8_t chan)
{
    this->IAudioOutput::SetOutputChannels(chan);

    // force reinit
    if (this->currentFormat.IsValid())
    {
        this->_init(this->currentFormat);
    }
}

void PortAudioOutput::init(SongFormat &format, bool realtime)
{
    if (format.IsValid())
    {
        if (this->currentFormat.SampleFormat == format.SampleFormat && this->currentFormat.SampleRate == format.SampleRate)
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

void PortAudioOutput::_init(SongFormat &format, bool)
{
    int channels = this->GetOutputChannels();
    PaSampleFormat paSampleFmt;
    switch (format.SampleFormat)
    {
        case SampleFormat_t::float32:
            paSampleFmt = paFloat32;
            this->processedBuffer.reserve(gConfig.FramesToRender * channels * sizeof(float));
            break;
        case SampleFormat_t::int16:
            paSampleFmt = paInt16;
            this->processedBuffer.reserve(gConfig.FramesToRender * channels * sizeof(int16_t));
            break;
        case SampleFormat_t::int32:
            paSampleFmt = paInt32;
            this->processedBuffer.reserve(gConfig.FramesToRender * channels * sizeof(int32_t));
            break;
        case SampleFormat_t::unknown:
            THROW_RUNTIME_ERROR("Sample Format not set");

        default:
            throw NotImplementedException();
            break;
    }

    this->drop();
    this->close();

    // Standard stream parameters
    PaStreamParameters outputParams;
    outputParams.device = d->deviceIndex;
    outputParams.channelCount = channels;
    outputParams.sampleFormat = paSampleFmt;
    outputParams.suggestedLatency = d->deviceInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr; // Will point to WASAPI struct.

    PaStream *stream = nullptr;

    PaError err;
    err = Pa_OpenStream(&d->handle,
                        nullptr, // no input
                        &outputParams,
                        d->deviceInfo->defaultSampleRate,
                        gConfig.FramesToRender,
                        paNoFlag,
                        nullptr,
                        nullptr);
    if (err != PaErrorCode::paNoError)
    {
        THROW_RUNTIME_ERROR("Pa_OpenStream failed (" << Pa_GetErrorText(err) << ")");
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
    if (d->handle != nullptr)
    {
        Pa_CloseStream(d->handle);
        d->handle = nullptr;
    }
}

int PortAudioOutput::write(const float *buffer, frame_t frames)
{
    return this->write<float>(buffer, frames);
}

int PortAudioOutput::write(const int16_t *buffer, frame_t frames)
{
    return this->write<int16_t>(buffer, frames);
}

int PortAudioOutput::write(const int32_t *buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
}

template<typename T>
int PortAudioOutput::write(const T *buffer, frame_t frames)
{
    if (d->handle == nullptr)
    {
        THROW_RUNTIME_ERROR("unable to write pcm since PortAudioOutput::init() has not been called yet or init failed");
    }
    
    auto* procBuf = reinterpret_cast<T*>(processedBuffer.data());
    this->Mix<T, T>(frames, buffer, this->currentFormat, procBuf);

    PaError err = Pa_WriteStream(d->handle, procBuf, frames);
    switch (err)
    {
        case PaErrorCode::paUnanticipatedHostError:
            return 0;
        case PaErrorCode::paInputOverflowed:
            [[fallthrough]];
        case PaErrorCode::paOutputUnderflowed:
            [[fallthrough]];
        case PaErrorCode::paNoError:
            return frames;
        default:
            THROW_RUNTIME_ERROR("unable to write pcm (" << Pa_GetErrorText(err) << ")");
    }
}

void PortAudioOutput::start()
{
    if (d->handle != nullptr)
    {
        PaError err = Pa_StartStream(d->handle);
        if (err != PaErrorCode::paNoError && err != paStreamIsNotStopped)
        {
            std::stringstream ss;
            if (err == paUnanticipatedHostError)
            {
                auto *errInfo = Pa_GetLastHostErrorInfo();
                ss << " Code " << errInfo->errorCode << ": '" << errInfo->errorText << "' | HostAPI: " << errInfo->hostApiType;
            }
            THROW_RUNTIME_ERROR("unable to start pcm (" << Pa_GetErrorText(err) << ")" << ss.str());
        }
    }
    else
    {
        THROW_RUNTIME_ERROR("unable to start pcm since PortAudioOutput::init() has not been called yet or init failed");
    }
}

void PortAudioOutput::stop()
{
    if (d->handle != nullptr)
    {
        // dont call Pa_StopStream() here since it causes draining the pcm, which takes time and may cause deadlocks
        // use Pa_AbortStream() instead which drops any PCM currently played
        PaError err = Pa_AbortStream(d->handle);
        if (err != PaErrorCode::paNoError && err != PaErrorCode::paStreamIsStopped)
        {
            THROW_RUNTIME_ERROR("unable to stop pcm (" << Pa_GetErrorText(err) << ")");
        }
    }
}
