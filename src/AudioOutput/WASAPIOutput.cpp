#ifdef _WIN32

#include "WASAPIOutput.h"
#include "IAudioOutput_impl.h"

#include "CommonExceptions.h"
#include "Config.h"
#include "AtomicWrite.h"
#include "types.h"

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
    bool started = false;

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
            std::cerr << "Failed to recover WASAPI device: " << e.what() << std::endl;
            return false;
        }
        catch (...)
        {
            std::cerr << "Failed to recover WASAPI device due to unknown error." << std::endl;
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
}

void WASAPIOutput::SetOutputChannels(uint8_t chan)
{
    this->IAudioOutput::SetOutputChannels(chan);
    if (this->currentFormat.IsValid())
    {
        this->init(this->currentFormat);
    }
}

void WASAPIOutput::init(SongFormat &format, bool)
{
    this->close();

    d->ensureEnumerator();

    bool wasStarted = d->started;

    HRESULT hr = d->enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &d->device);
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
                                         reinterpret_cast<WAVEFORMATEX *>(&desired),
                                         &closest);
    DWORD flags;
    WAVEFORMATEX *finalFormat = nullptr;
    if (hr == S_OK)
    {
        finalFormat = reinterpret_cast<WAVEFORMATEX *>(&desired);
    }
    else if (hr == S_FALSE && closest != nullptr)
    {
        CLOG(LogLevel_t::Info, "wasapi: requested mode cannot be fully satisfied.");

        if (closest->nSamplesPerSec != desired.Format.nSamplesPerSec) // needs resampling
        {
            flags = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
            OSVERSIONINFOEXW vi = {sizeof(vi), 6, 0, 0, 0, {0}, 0, 0, 0, 0, 0};
            vi.dwMinorVersion = 1;

            if (VerifyVersionInfoW(&vi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR,
                                   VerSetConditionMask(VerSetConditionMask(VerSetConditionMask(0,
                                                                                               VER_MAJORVERSION, VER_GREATER_EQUAL),
                                                                           VER_MINORVERSION, VER_GREATER_EQUAL),
                                                       VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL)))
            // IAudioClient::Initialize in Vista fails with E_INVALIDARG if this flag is set
            {
                flags |= AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
                finalFormat = &desired.Format;
            }
        }
        if (finalFormat == nullptr)
        {
            finalFormat = closest;
        }
    }
    else
    {
        finalFormat = mixFormat;
    }

    if (finalFormat->nChannels != this->GetOutputChannels())
    {
        std::cerr << "Requested " << static_cast<int>(this->GetOutputChannels()) << " channels, using "
                  << finalFormat->nChannels << " as provided by WASAPI." << std::endl;
        this->IAudioOutput::SetOutputChannels(finalFormat->nChannels);
    }

    REFERENCE_TIME bufferDuration = static_cast<REFERENCE_TIME>(gConfig.PreRenderTime) * 10000;
    hr = d->client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  flags,
                                  bufferDuration,
                                  0,
                                  finalFormat,
                                  nullptr);

    if (FAILED(hr))
    {
        if (closest != nullptr)
        {
            CoTaskMemFree(closest);
        }
        CoTaskMemFree(mixFormat);
        THROW_RUNTIME_ERROR("unable to initialize WASAPI client (" << std::hex << hr << ")");
    }

    hr = d->client->GetBufferSize(&d->bufferFrameCount);
    if (FAILED(hr))
    {
        if (closest != nullptr)
        {
            CoTaskMemFree(closest);
        }
        CoTaskMemFree(mixFormat);
        THROW_RUNTIME_ERROR("unable to get WASAPI buffer size (" << std::hex << hr << ")");
    }

    hr = d->client->GetService(IID_PPV_ARGS(&d->renderClient));
    if (FAILED(hr))
    {
        if (closest != nullptr)
        {
            CoTaskMemFree(closest);
        }
        CoTaskMemFree(mixFormat);
        THROW_RUNTIME_ERROR("unable to get render client (" << std::hex << hr << ")");
    }

    if (closest != nullptr)
    {
        CoTaskMemFree(closest);
    }
    CoTaskMemFree(mixFormat);

    const size_t requiredSize = gConfig.FramesToRender * this->GetOutputChannels() * sizeof(float);
    if (this->processedBuffer.size() != requiredSize)
    {
        this->processedBuffer.resize(requiredSize);
    }
    this->currentFormat = format;

    if (wasStarted)
    {
        this->start();
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

    UINT32 padding = 0;
    HRESULT hr = d->client->GetCurrentPadding(&padding);
    if (hr == AUDCLNT_E_DEVICE_INVALIDATED && d->recoverDevice())
    {
        return 0;
    }
    if (FAILED(hr))
    {
        THROW_RUNTIME_ERROR("GetCurrentPadding failed (" << std::hex << hr << ")");
    }

    UINT32 available = d->bufferFrameCount - padding;
    if (available == 0)
    {
        return 0;
    }

    UINT32 framesToWrite = std::min<UINT32>(frames, available);
    BYTE *data = nullptr;
    hr = d->renderClient->GetBuffer(framesToWrite, &data);
    if (hr == AUDCLNT_E_DEVICE_INVALIDATED && d->recoverDevice())
    {
        return 0;
    }
    if (FAILED(hr))
    {
        THROW_RUNTIME_ERROR("GetBuffer failed (" << std::hex << hr << ")");
    }

    std::memcpy(data, procBuf, framesToWrite * this->GetOutputChannels() * sizeof(float));

    hr = d->renderClient->ReleaseBuffer(framesToWrite, 0);
    if (hr == AUDCLNT_E_DEVICE_INVALIDATED && d->recoverDevice())
    {
        return 0;
    }
    if (FAILED(hr))
    {
        THROW_RUNTIME_ERROR("ReleaseBuffer failed (" << std::hex << hr << ")");
    }

    return static_cast<int>(framesToWrite);
}

void WASAPIOutput::start()
{
    if (!d->client)
    {
        THROW_RUNTIME_ERROR("unable to start pcm since WASAPIOutput::init() has not been called yet or init failed");
    }
    
    // https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-start
    // To avoid start-up glitches with rendering streams, clients should not call Start until the audio engine has been initially loaded with data by calling the
    // IAudioRenderClient::GetBuffer and IAudioRenderClient::ReleaseBuffer methods on the rendering interface.
    BYTE *data = nullptr;

    UINT32 framesToWrite = d->bufferFrameCount;
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

    std::memset(data, 0, framesToWrite * this->GetOutputChannels() * sizeof(float));

    hr = d->renderClient->ReleaseBuffer(framesToWrite, 0);
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
    if (d->client)
    {
        HRESULT hr = d->client->Stop();
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED && d->recoverDevice())
        {
            d->started = false;
            return;
        }
        if (FAILED(hr) && hr != AUDCLNT_E_NOT_INITIALIZED)
        {
            THROW_RUNTIME_ERROR("unable to stop pcm (" << std::hex << hr << ")");
        }
    }
    d->started = false;
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
