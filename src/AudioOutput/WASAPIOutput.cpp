#ifdef _WIN32

#include "WASAPIOutput.h"
#include "IAudioOutput_impl.h"

#include "CommonExceptions.h"
#include "Config.h"

#include <audioclient.h>
#include <combaseapi.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <algorithm>
#include <cstring>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

WASAPIOutput::WASAPIOutput()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE)
    {
        this->comInitialized = SUCCEEDED(hr);
    }
    this->processedBuffer.resize(gConfig.FramesToRender * this->GetOutputChannels() * sizeof(float));
}

WASAPIOutput::~WASAPIOutput()
{
    this->close();
    if (this->comInitialized)
    {
        CoUninitialize();
    }
}

void WASAPIOutput::ensureEnumerator()
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

void WASAPIOutput::open()
{
    this->ensureEnumerator();
}

void WASAPIOutput::SetOutputChannels(uint8_t chan)
{
    this->IAudioOutput::SetOutputChannels(chan);
    if (this->currentFormat.IsValid())
    {
        this->init(this->currentFormat);
    }
}

WAVEFORMATEXTENSIBLE WASAPIOutput::buildWaveFormat(const SongFormat &format) const
{
    WAVEFORMATEXTENSIBLE wfx = {};
    wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfx.Format.nChannels = this->GetOutputChannels();
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

void WASAPIOutput::init(SongFormat &format, bool)
{
    this->ensureEnumerator();

    this->close();

    HRESULT hr = this->enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &this->device);
    if (FAILED(hr))
    {
        THROW_RUNTIME_ERROR("cannot get default audio endpoint (" << std::hex << hr << ")");
    }

    hr = this->device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &this->client);
    if (FAILED(hr))
    {
        THROW_RUNTIME_ERROR("cannot activate audio client (" << std::hex << hr << ")");
    }

    WAVEFORMATEX *mixFormat = nullptr;
    hr = this->client->GetMixFormat(&mixFormat);
    if (FAILED(hr) || mixFormat == nullptr)
    {
        THROW_RUNTIME_ERROR("cannot get mix format (" << std::hex << hr << ")");
    }

    WAVEFORMATEXTENSIBLE desired = this->buildWaveFormat(format);
    WAVEFORMATEX *closest = nullptr;
    hr = this->client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,
                                         reinterpret_cast<WAVEFORMATEX *>(&desired),
                                         &closest);

    WAVEFORMATEX *finalFormat = nullptr;
    if (hr == S_OK)
    {
        finalFormat = reinterpret_cast<WAVEFORMATEX *>(&desired);
    }
    else if (hr == S_FALSE && closest != nullptr)
    {
        finalFormat = closest;
    }
    else
    {
        finalFormat = mixFormat;
    }

    this->IAudioOutput::SetOutputChannels(finalFormat->nChannels);

    REFERENCE_TIME bufferDuration = static_cast<REFERENCE_TIME>(gConfig.PreRenderTime) * 10000;
    hr = this->client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
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

    hr = this->client->GetBufferSize(&this->bufferFrameCount);
    if (FAILED(hr))
    {
        if (closest != nullptr)
        {
            CoTaskMemFree(closest);
        }
        CoTaskMemFree(mixFormat);
        THROW_RUNTIME_ERROR("unable to get WASAPI buffer size (" << std::hex << hr << ")");
    }

    hr = this->client->GetService(IID_PPV_ARGS(&this->renderClient));
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

    this->processedBuffer.resize(gConfig.FramesToRender * this->GetOutputChannels() * sizeof(float));
    this->currentFormat = format;
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
    if (!this->client || !this->renderClient)
    {
        THROW_RUNTIME_ERROR("unable to write pcm since WASAPIOutput::init() has not been called yet or init failed");
    }

    auto *procBuf = reinterpret_cast<float *>(processedBuffer.data());
    this->Mix<T, float>(frames, buffer, this->currentFormat, procBuf);

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
    if (available == 0)
    {
        return 0;
    }

    UINT32 framesToWrite = std::min<UINT32>(frames, available);
    BYTE *data = nullptr;
    hr = this->renderClient->GetBuffer(framesToWrite, &data);
    if (hr == AUDCLNT_E_DEVICE_INVALIDATED && this->recoverDevice())
    {
        return 0;
    }
    if (FAILED(hr))
    {
        THROW_RUNTIME_ERROR("GetBuffer failed (" << std::hex << hr << ")");
    }

    std::memcpy(data, procBuf, framesToWrite * this->GetOutputChannels() * sizeof(float));

    hr = this->renderClient->ReleaseBuffer(framesToWrite, 0);
    if (hr == AUDCLNT_E_DEVICE_INVALIDATED && this->recoverDevice())
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
    if (!this->client)
    {
        THROW_RUNTIME_ERROR("unable to start pcm since WASAPIOutput::init() has not been called yet or init failed");
    }

    this->started = true;
    HRESULT hr = this->client->Start();
    if (hr == AUDCLNT_E_DEVICE_INVALIDATED && this->recoverDevice())
    {
        return;
    }
    if (FAILED(hr) && hr != AUDCLNT_E_NOT_INITIALIZED)
    {
        this->started = false;
        THROW_RUNTIME_ERROR("unable to start pcm (" << std::hex << hr << ")");
    }
    this->started = true;
}

void WASAPIOutput::stop()
{
    if (this->client)
    {
        HRESULT hr = this->client->Stop();
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED && this->recoverDevice())
        {
            this->started = false;
            return;
        }
        if (FAILED(hr) && hr != AUDCLNT_E_NOT_INITIALIZED)
        {
            THROW_RUNTIME_ERROR("unable to stop pcm (" << std::hex << hr << ")");
        }
    }
    this->started = false;
}

void WASAPIOutput::close()
{
    this->renderClient.Reset();
    this->client.Reset();
    this->device.Reset();
    this->bufferFrameCount = 0;
    this->started = false;
}

bool WASAPIOutput::recoverDevice()
{
    if (!this->currentFormat.IsValid())
    {
        return false;
    }

    bool wasStarted = this->started;
    try
    {
        this->init(this->currentFormat);
        if (wasStarted)
        {
            this->start();
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

#endif // _WIN32
