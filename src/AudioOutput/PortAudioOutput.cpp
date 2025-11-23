#include "PortAudioOutput.h"
#include "IAudioOutput_impl.h"

#include "AtomicWrite.h"
#include "CommonExceptions.h"
#include "Config.h"

#include <iostream>
#include <string>
#include <portaudio.h>
#include <samplerate.h>


struct PortAudioOutput::Impl
{    
    PaStream *handle = nullptr;
    PaDeviceIndex deviceIndex;
    const PaDeviceInfo *deviceInfo;

    SRC_STATE* srcState;
    SRC_DATA srcData;

    // mixed, converted to float and resampled buffer used in this->write()
    std::vector<float> resampledBuffer;
};

PortAudioOutput::PortAudioOutput() : d(std::make_unique<Impl>())
{
    auto paInitError = Pa_Initialize();
    if (paInitError != PaErrorCode::paNoError)
    {
        THROW_RUNTIME_ERROR("unable to initialize portaudio (" << Pa_GetErrorText(paInitError) << ")");
    }
#ifdef _WIN32
    // Find WASAPI host API index, the default MME backend is broken somehow, it randomly fails when switching between songs
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
        this->SetOutputChannels(this->GetOutputChannels());
    }
}


void PortAudioOutput::SetOutputChannels(uint8_t chan)
{
    this->IAudioOutput::SetOutputChannels(chan);

    // we have to delete the resampler in order to refresh channel count
    if (d->srcState != nullptr)
    {
        d->srcState = src_delete(d->srcState);
    }
    int error;
    // SRC_SINC_BEST_QUALITY is too slow, causing jack process thread to discard samples, resulting in hearable artifacts
    // SRC_LINEAR has high frequency audible garbage
    // SRC_ZERO_ORDER_HOLD is even worse than LINEAR
    // SRC_SINC_MEDIUM_QUALITY might still be too slow when using jack with very low latency having a bit of CPU load
    d->srcState = src_new(SRC_LINEAR, chan, &error);
    if (d->srcState == nullptr)
    {
        THROW_RUNTIME_ERROR("unable to init libsamplerate (" << src_strerror(error) << ")");
    }

    this->d->resampledBuffer.reserve(gConfig.FramesToRender * chan);

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
            this->processedBuffer.reserve(gConfig.FramesToRender * channels * sizeof(float));
            break;
        case SampleFormat_t::int16:
            this->processedBuffer.reserve(gConfig.FramesToRender * channels * sizeof(int16_t));
            break;
        case SampleFormat_t::int32:
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

    // zero out any buffer in resampler, to avoid hearable cracks, when switching from one song to another
    src_reset(d->srcState);

    // Standard stream parameters
    PaStreamParameters outputParams;
    outputParams.device = d->deviceIndex;
    outputParams.channelCount = channels;
    outputParams.sampleFormat = paFloat32;
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

    this->start();
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
    
    auto* procBuf = reinterpret_cast<float*>(processedBuffer.data());
    this->Mix<T, float>(frames, buffer, this->currentFormat, procBuf);
    
    auto framesUsedNow = this->doResampling(procBuf, frames);

    PaError err = Pa_WriteStream(d->handle, d->resampledBuffer.data(), framesUsedNow);
    switch (err)
    {
        case PaErrorCode::paUnanticipatedHostError:
            return 0;
        case PaErrorCode::paInputOverflowed:
            CLOG(LogLevel_t::Info, "overflow");
            [[fallthrough]];
        case PaErrorCode::paOutputUnderflowed:
            CLOG(LogLevel_t::Info, "underflow");
            [[fallthrough]];
        case PaErrorCode::paNoError:
            return framesUsedNow;
        default:
            THROW_RUNTIME_ERROR("unable to write pcm (" << Pa_GetErrorText(err) << ")");
    }
}

int PortAudioOutput::doResampling(const float *inBuf, const size_t Frames)
{
    d->srcData.data_in = inBuf;
    d->srcData.input_frames = Frames;

    d->srcData.data_out = d->resampledBuffer.data();

    d->srcData.output_frames = gConfig.FramesToRender - d->srcData.output_frames_gen;

    // remember the count of frames the already have been resampled
    int old = d->srcData.output_frames_gen;

    // output_sample_rate / input_sample_rate
    d->srcData.src_ratio = (double)d->deviceInfo->defaultSampleRate / this->currentFormat.SampleRate;

    int err = src_process(d->srcState, &d->srcData);
    if (err != 0)
    {
        CLOG(LogLevel_t::Error, "libsamplerate failed processing (" << src_strerror(err) << ")");
    }
    else
    {
        if (d->srcData.output_frames_gen < d->srcData.output_frames)
        {
            CLOG(LogLevel_t::Info, "resample buffer has not been filled completely" << std::endl
                                << "input_frames: " << d->srcData.input_frames << "\toutput_frames: " << d->srcData.output_frames << std::endl
                                << "input_frames_used: " << d->srcData.input_frames_used << "\toutput_frames_gen: " << d->srcData.output_frames_gen);

            // needed next time to advance data_out
            d->srcData.output_frames_gen += old;
        }
        else
        {
            d->srcData.output_frames_gen = 0;
        }
    }
    return d->srcData.input_frames_used;
}

void PortAudioOutput::start()
{
    // zero out any buffer in resampler, to avoid hearable cracks, when pausing and restarting playback
    src_reset(d->srcState);

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
