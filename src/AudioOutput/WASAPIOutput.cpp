#ifdef _WIN32

#include "WASAPIOutput.h"
#include "IAudioOutput_impl.h"

#include "CommonExceptions.h"
#include "Config.h"
#include "AtomicWrite.h"
#include "types.h"
#include "ringbuffer.hpp"

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
#include <mutex>

// {12c8e646-7548-4f13-8a5c-cfa64db5c468}
inline constexpr GUID GUID_ANMP_SESSION = {0x12c8e646, 0x7548, 0x4f13, {0x8a, 0x5c, 0xcf, 0xa6, 0x4d, 0xb5, 0xc4, 0x68}};

using Microsoft::WRL::ComPtr;

struct WASAPIOutput::Impl
{
    WASAPIOutput *q;
    HANDLE needDataEvent = nullptr;

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

    jnk0le::Ringbuffer<float, gConfig.FramesToRender * 2, false, 64, uint16_t> interleavedProcessedBuffer;

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
        HRESULT hr = this->client->GetCurrentPadding(&padding);
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED && this->recoverDevice())
        {
            return 0;
        }
        if (FAILED(hr))
        {
            THROW_RUNTIME_ERROR("GetCurrentPadding failed (" << std::hex << hr << ")");
        }

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

    d->needDataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (d->needDataEvent == nullptr)
    {
        THROW_RUNTIME_ERROR("Failed to create event");
    }
}

WASAPIOutput::~WASAPIOutput()
{
    this->close();
    if (d->needDataEvent != nullptr)
    {
        CloseHandle(d->needDataEvent);
    }
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
        hr = d->enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &d->device);
        if (FAILED(hr))
        {
            THROW_RUNTIME_ERROR("cannot get default audio endpoint (" << std::hex << hr << ")");
        }

        hr = d->device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &d->client);
        if (FAILED(hr))
        {
            THROW_RUNTIME_ERROR("cannot activate audio client (" << std::hex << hr << ")");
        }

        WAVEFORMATEX *mixFormat = nullptr;
        hr = d->client->GetMixFormat(&mixFormat);
        if (FAILED(hr) || mixFormat == nullptr)
        {
            THROW_RUNTIME_ERROR("cannot get mix format (" << std::hex << hr << ")");
        }

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
            CLOG(LogLevel_t::Info, "wasapi: requested mode cannot be fully satisfied.");

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

        REFERENCE_TIME bufferDuration = static_cast<REFERENCE_TIME>(gConfig.FramesToRender * 4.0 / format.SampleRate * 1000 * 1000 * 10 + 0.5);
        hr = d->client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                   0,
                                   bufferDuration,
                                   0,
                                   finalFormat,
                                   &GUID_ANMP_SESSION);

        d->deviceSampleRate = finalFormat->nSamplesPerSec;
        CoTaskMemFree(closest);
        CoTaskMemFree(mixFormat);

        if (FAILED(hr))
        {
            THROW_RUNTIME_ERROR("unable to initialize WASAPI client (" << std::hex << hr << ")");
        }

        //hr = d->client->SetEventHandle(d->needDataEvent);
        //if (FAILED(hr))
        //{
        //    THROW_RUNTIME_ERROR("failed to set event handle (" << std::hex << hr << ")");
        //}

        hr = d->client->GetBufferSize(&d->bufferFrameCount);
        if (FAILED(hr))
        {
            THROW_RUNTIME_ERROR("unable to get WASAPI buffer size (" << std::hex << hr << ")");
        }

        hr = d->client->GetService(IID_PPV_ARGS(&d->renderClient));
        if (FAILED(hr))
        {
            THROW_RUNTIME_ERROR("unable to get render client (" << std::hex << hr << ")");
        }

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
        //auto ret = WaitForSingleObject(d->needDataEvent, 2000);
        //if (ret != WAIT_OBJECT_0)
        //{
        //    THROW_RUNTIME_ERROR("Waiting for event timed out!");
        //}
        auto available = d->GetAvailableFrames();
        if (available == 0)
        {
            //CLOG(LogLevel_t::Info, "No space available in buffer, sleeping");
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((d->bufferFrameCount / d->deviceSampleRate) * (1000 / 2))));

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

        auto hr = d->renderClient->GetBuffer(framesToWrite, &data);
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED && d->recoverDevice())
        {
            return framesConsumed;
        }
        if (FAILED(hr))
        {
            THROW_RUNTIME_ERROR("GetBuffer failed (" << std::hex << hr << ")");
        }

        int framesConsumedNow;

        d->doResampling(procBuf + framesConsumed * this->GetOutputChannels(), frames, reinterpret_cast<float*>(data), framesToWrite);
        //std::memcpy(data, procBuf + framesConsumed * this->GetOutputChannels(), framesToWrite * sizeof(float) * this->GetOutputChannels());
        framesConsumedNow = d->srcData.input_frames_used;
        frames -= framesConsumedNow;
        framesConsumed += framesConsumedNow;

        hr = d->renderClient->ReleaseBuffer(d->srcData.output_frames_gen, 0);
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED && d->recoverDevice())
        {
            return framesConsumed;
        }
        if (FAILED(hr))
        {
            THROW_RUNTIME_ERROR("ReleaseBuffer failed (" << std::hex << hr << ")");
        }

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

    d->client->Reset();
    auto hr = d->renderClient->GetBuffer(framesToWrite, &data);
    if (hr == AUDCLNT_E_DEVICE_INVALIDATED && !d->recoverDevice())
    {
        THROW_RUNTIME_ERROR("unable to start pcm AUDCLNT_E_DEVICE_INVALIDATED and failed to recover device");
    }
    if (FAILED(hr))
    {
        THROW_RUNTIME_ERROR("GetBuffer failed (" << std::hex << hr << ")");
    }

    hr = d->renderClient->ReleaseBuffer(framesToWrite, AUDCLNT_BUFFERFLAGS_SILENT);
    if (hr == AUDCLNT_E_DEVICE_INVALIDATED && !d->recoverDevice())
    {
        // ignore
    }
    if (FAILED(hr))
    {
        THROW_RUNTIME_ERROR("ReleaseBuffer failed (" << std::hex << hr << ")");
    }

    d->started = true;
    hr = d->client->Start();
    if (hr == AUDCLNT_E_DEVICE_INVALIDATED && d->recoverDevice())
    {
        return;
    }
    if (FAILED(hr) && hr != AUDCLNT_E_NOT_INITIALIZED)
    {
        d->started = false;
        THROW_RUNTIME_ERROR("unable to start pcm (" << std::hex << hr << ")");
    }
}

void WASAPIOutput::stop()
{
    std::lock_guard m(d->mtxBufferOperationInProgress);

    d->started = false;
    if (d->client)
    {
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
