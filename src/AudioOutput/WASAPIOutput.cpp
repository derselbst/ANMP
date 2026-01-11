#ifdef _WIN32

#include "WASAPIOutput.h"
#include "IAudioOutput_impl.h"

#include "CommonExceptions.h"
#include "Config.h"
#include "AtomicWrite.h"
#include "types.h"
#include "ringbuffer.hpp"
#include "StringFormatter.h"

#include <audioclient.h>
#include <combaseapi.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <wrl/client.h>
#include <samplerate.h>
#include <guiddef.h>
#include <comdef.h>
#include <mutex>

// {12c8e646-7548-4f13-8a5c-cfa64db5c468}
inline constexpr GUID GUID_ANMP_SESSION = {0x12c8e646, 0x7548, 0x4f13, {0x8a, 0x5c, 0xcf, 0xa6, 0x4d, 0xb5, 0xc4, 0x68}};

inline constexpr float NumberOfPeriods = 4.0f;

using Microsoft::WRL::ComPtr;

struct WASAPIOutput::Impl
{
    WASAPIOutput *q;

    ComPtr<IMMDeviceEnumerator> enumerator;
    ComPtr<IMMDevice> device;
    ComPtr<IAudioClient> client;
    ComPtr<IAudioRenderClient> renderClient;

    UINT32 bufferFrameCount = 0;
    bool comInitialized = false;

    std::mutex mtxBufferOperationInProgress;
    bool started = false;

    float deviceSampleRate = 0;

    SRC_STATE *srcState = nullptr;
    SRC_DATA srcData;

    Impl(WASAPIOutput *parent)
    : q(parent)
    {
    }

    void ensureEnumerator()
    {
        if (this->enumerator)
        {
            return;
        }

        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&this->enumerator));
        if (FAILED(hr))
        {
            THROW_RUNTIME_ERROR("cannot create MMDeviceEnumerator (" << std::hex << hr << ")");
        }
    }

    bool recoverDevice()
    {
        if (!q->currentFormat.IsValid())
        {
            return false;
        }

        bool wasStarted = this->started;
        try
        {
            SongFormat formatCopy = q->currentFormat;
            q->init(formatCopy);
            if (wasStarted)
            {
                q->start();
            }
            return true;
        }
        catch (const std::exception &e)
        {
            CLOG(LogLevel_t::Error, "Failed to recover WASAPI device: " << e.what());
            return false;
        }
        catch (...)
        {
            CLOG(LogLevel_t::Error, "Failed to recover WASAPI device due to unknown error.");
            return false;
        }
    }


    WAVEFORMATEXTENSIBLE buildWaveFormat(const SongFormat &format, int outputChannels) const
    {
        WAVEFORMATEXTENSIBLE wfx = {};
        wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx.Format.nChannels = outputChannels;
        wfx.Format.nSamplesPerSec = format.SampleRate;
        wfx.Format.wBitsPerSample = 32;
        wfx.Format.nBlockAlign = (wfx.Format.nChannels * wfx.Format.wBitsPerSample) / 8;
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        wfx.Samples.wValidBitsPerSample = 32;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        switch (wfx.Format.nChannels)
        {
            case 1:
                wfx.dwChannelMask = SPEAKER_FRONT_CENTER;
                break;
            case 2:
                wfx.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
                break;
            default:
                wfx.dwChannelMask = 0;
                break;
        }

        return wfx;
    }

    UINT32 GetAvailableFrames()
    {
        UINT32 padding = 0;
        this->guardedCall([&] {
            return this->client->GetCurrentPadding(&padding); },
            true,
            "GetCurrentPadding failed");

        UINT32 available = this->bufferFrameCount - padding;
        return available;
    }

    void doResampling(const float *inBuf, const frame_t Frames, float *outBuf, int framesAvailable)
    {
        if (!this->started)
        {
            THROW_RUNTIME_ERROR("Stream is stopped")
        }
        if (!q->currentFormat.IsValid())
        {
            THROW_RUNTIME_ERROR("SongFormat not valid")
        }

        this->srcData.data_in = inBuf;
        this->srcData.input_frames = Frames;

        this->srcData.data_out = outBuf;
        this->srcData.output_frames = framesAvailable;

        this->srcData.end_of_input = false;

        // output_sample_rate / input_sample_rate
        this->srcData.src_ratio = this->deviceSampleRate/ q->currentFormat.SampleRate;

        int err = src_process(this->srcState, &this->srcData);
        if (err != 0)
        {
            THROW_RUNTIME_ERROR("libsamplerate failed processing (" << src_strerror(err) << ")");
        }
        else
        {
            #if 0
            if (this->srcData.input_frames_used < Frames)
            {
                CLOG(LogLevel_t::Info, "Not all input frames used!"
                                       << "input_frames: " << this->srcData.input_frames << "\toutput_frames: " << this->srcData.output_frames << std::endl
                                       << "input_frames_used: " << this->srcData.input_frames_used << "\toutput_frames_gen: " << this->srcData.output_frames_gen);
            }

            if (this->srcData.output_frames_gen < this->srcData.output_frames)
            {
                CLOG(LogLevel_t::Info, "resample buffer has not been filled completely" << std::endl
                                                                                        << "input_frames: " << this->srcData.input_frames << "\toutput_frames: " << this->srcData.output_frames << std::endl
                                                                                        << "input_frames_used: " << this->srcData.input_frames_used << "\toutput_frames_gen: " << this->srcData.output_frames_gen);
            }
            #endif
        }
    }

    template<typename T>
    auto guardedCall(T &&func, bool recoverDeviceEnabled, const char *operationError) -> void
    {
        HRESULT hr = func();
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED)
        {
            if (recoverDeviceEnabled && this->recoverDevice())
            {
                hr = func();
            }
        }
        std::stringstream ss;
        if (FAILED(hr))
        {
            auto friendlyError = StringFormatter::GetLastWinError(hr);

            ss << "WASAPI: " << operationError << ". Error: 0x" << std::hex << hr << " - " << friendlyError;
            THROW_RUNTIME_ERROR(ss.str());
        }
    }
};

WASAPIOutput::WASAPIOutput() : d(std::make_unique<Impl>(this))
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool comReady = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
    d->comInitialized = SUCCEEDED(hr);
    if (!comReady)
    {
        THROW_RUNTIME_ERROR("CoInitializeEx failed (" << std::hex << hr << ")");
    }
}

WASAPIOutput::~WASAPIOutput()
{
    this->close();
    if (d->comInitialized)
    {
        CoUninitialize();
    }
}

void WASAPIOutput::open()
{
    d->ensureEnumerator();
    if (d->client == nullptr)
    {
        this->SetOutputChannels(this->GetOutputChannels());
    }
}

void WASAPIOutput::SetOutputChannels(uint8_t chan)
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
    d->srcState = src_new(SRC_SINC_FASTEST, chan, &error);
    if (d->srcState == nullptr)
    {
        THROW_RUNTIME_ERROR("unable to init libsamplerate (" << src_strerror(error) << ")");
    }

    if (this->currentFormat.IsValid())
    {
        this->init(this->currentFormat);
    }
}

void WASAPIOutput::init(SongFormat &format, bool)
{
    d->ensureEnumerator();
    
    HRESULT hr;
    bool clientWasNull;
    if (clientWasNull = (d->client == nullptr))
    {
        d->guardedCall([&] { return d->enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &d->device); },
                       false,
                       "Cannot get default audio endpoint");

        d->guardedCall([&] { return d->device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &d->client); },
            false,
            "Cannot activate audio client");

        WAVEFORMATEX *mixFormat = nullptr;
        d->guardedCall([&] { return d->client->GetMixFormat(&mixFormat);},
            false,
            "Cannot get mix format");

        WAVEFORMATEXTENSIBLE desired = d->buildWaveFormat(format, this->GetOutputChannels());
        WAVEFORMATEX *closest = nullptr;
        hr = d->client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,
                                          &desired.Format,
                                          &closest);
        DWORD flags;
        WAVEFORMATEX *finalFormat = nullptr;
        if (hr == S_OK)
        {
            finalFormat = &desired.Format;
        }
        else if (hr == S_FALSE && closest != nullptr)
        {
            CLOG(LogLevel_t::Debug, "wasapi: requested mode cannot be fully satisfied.");

            finalFormat = closest;
        }
        else
        {
            finalFormat = mixFormat;
        }

        if (finalFormat->nChannels != this->GetOutputChannels())
        {
            CLOG(LogLevel_t::Error, "Requested " << static_cast<int>(this->GetOutputChannels()) << " channels, using "
                      << finalFormat->nChannels << " as provided by WASAPI.");
            this->IAudioOutput::SetOutputChannels(finalFormat->nChannels);
        }

        REFERENCE_TIME bufferDuration = static_cast<REFERENCE_TIME>(gConfig.FramesToRender * NumberOfPeriods / format.SampleRate * 1000 * 1000 * 10 + 0.5);
        d->guardedCall([&]
        {
            auto hr = d->client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                            0,
                                            bufferDuration,
                                            0,
                                            finalFormat,
                                            &GUID_ANMP_SESSION);

            d->deviceSampleRate = finalFormat->nSamplesPerSec;
            CoTaskMemFree(closest);
            CoTaskMemFree(mixFormat);
            return hr;
        },
        false,
        "Unable to initialize audio client");

        d->guardedCall([&] { return d->client->GetBufferSize(&d->bufferFrameCount); },
                       false,
                       "Unable to get buffer size");

        d->guardedCall([&] { return d->client->GetService(IID_PPV_ARGS(&d->renderClient)); },
                       false,
                       "Unable to get render client");

        const size_t requiredSize = gConfig.FramesToRender * this->GetOutputChannels() * sizeof(float);
        this->processedBuffer.resize(requiredSize);
    }

    this->currentFormat = format;
    auto& srate = this->currentFormat.SampleRate;

    if (srate == 0)
    {
        src_set_ratio(d->srcState, 1);
        srate = d->deviceSampleRate;
    }
    else
    {
        src_set_ratio(d->srcState, d->deviceSampleRate / srate);
    }
}

int WASAPIOutput::write(const float *buffer, frame_t frames)
{
    return this->writeInternal<float>(buffer, frames);
}

int WASAPIOutput::write(const int16_t *buffer, frame_t frames)
{
    return this->writeInternal<int16_t>(buffer, frames);
}

int WASAPIOutput::write(const int32_t *buffer, frame_t frames)
{
    return this->writeInternal<int32_t>(buffer, frames);
}

template<typename T>
int WASAPIOutput::writeInternal(const T *buffer, frame_t frames)
{
    if (!d->client || !d->renderClient)
    {
        THROW_RUNTIME_ERROR("unable to write pcm since WASAPIOutput::init() has not been called yet or init failed");
    }

    auto *procBuf = reinterpret_cast<float *>(processedBuffer.data());
    this->Mix<T, float>(frames, buffer, this->currentFormat, procBuf);

    int framesConsumed = 0;

    std::unique_lock m(d->mtxBufferOperationInProgress, std::defer_lock);
    do
    {
        auto available = d->GetAvailableFrames();
        if (available == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((d->bufferFrameCount / d->deviceSampleRate) * (1000 / NumberOfPeriods))));

            available = d->GetAvailableFrames();
            if (available == 0)
            {
                CLOG(LogLevel_t::Warning, "Despite waiting still no space available in audio buffer!!");
                return framesConsumed;
            }
        }

        UINT32 framesToWrite = std::min<UINT32>(frames, available);
        BYTE *data = nullptr;

        m.lock();
        if (!d->started)
        {
            return framesConsumed;
        }

        d->guardedCall([&] { return d->renderClient->GetBuffer(framesToWrite, &data); },
            true,
            "GetBuffer failed");

        d->doResampling(procBuf + framesConsumed * this->GetOutputChannels(), frames, reinterpret_cast<float*>(data), framesToWrite);
        int framesConsumedNow = d->srcData.input_frames_used;
        frames -= framesConsumedNow;
        framesConsumed += framesConsumedNow;

        d->guardedCall([&] {
            return d->renderClient->ReleaseBuffer(d->srcData.output_frames_gen, 0); },
            true,
            "ReleaseBuffer failed");

        m.unlock();
    } while (frames > 0);

    return framesConsumed;
}

void WASAPIOutput::start()
{
    if (!d->client)
    {
        THROW_RUNTIME_ERROR("unable to start pcm since WASAPIOutput::init() has not been called yet or init failed");
    }

    std::lock_guard m(d->mtxBufferOperationInProgress);
    
    // https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-start
    // To avoid start-up glitches with rendering streams, clients should not call Start until the audio engine has been initially loaded with data by calling the
    // IAudioRenderClient::GetBuffer and IAudioRenderClient::ReleaseBuffer methods on the rendering interface.
    BYTE *data = nullptr;

    UINT32 framesToWrite = d->bufferFrameCount;

    // zero out any buffer in resampler, to avoid hearable cracks, when pausing and restarting playback
    src_reset(d->srcState);

    d->guardedCall([&] { return d->client->Reset(); },
                   true,
                   "Reseting audio client failed");

    d->guardedCall([&] { return d->renderClient->GetBuffer(framesToWrite, &data); },
                   true,
                   "GetBuffer for prepopulation failed");

    d->guardedCall([&] {
        return d->renderClient->ReleaseBuffer(framesToWrite, AUDCLNT_BUFFERFLAGS_SILENT); },
        true,
        "ReleaseBuffer for prepopulation failed");

    d->guardedCall([&] {
        return d->client->Start(); },
        true,
        "Unable to start audio stream");
    d->started = true;
}

void WASAPIOutput::stop()
{
    std::lock_guard m(d->mtxBufferOperationInProgress);

    d->started = false;
    if (d->client)
    {
        // Intentionally no guardedCall here due to different error handling
        HRESULT hr = d->client->Stop();
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED && d->recoverDevice())
        {
            return;
        }
        if (FAILED(hr) && hr != AUDCLNT_E_NOT_INITIALIZED)
        {
            THROW_RUNTIME_ERROR("unable to stop pcm (" << std::hex << hr << ")");
        }
    }
}

void WASAPIOutput::close()
{
    d->renderClient.Reset();
    d->renderClient = nullptr;
    d->client.Reset();
    d->client = nullptr;
    d->device.Reset();
    d->device = nullptr;
    d->enumerator = nullptr;
    d->bufferFrameCount = 0;
    d->started = false;
}


#endif // _WIN32
