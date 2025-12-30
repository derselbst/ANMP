#include "PortAudioOutput.h"
#include "IAudioOutput_impl.h"

#include "AtomicWrite.h"
#include "CommonExceptions.h"
#include "Config.h"

#include <iostream>
#include <string>
#include <portaudio.h>
#include <samplerate.h>
#include <mutex>
#include <condition_variable>


struct PortAudioOutput::Impl
{    
    PaStream *handle = nullptr;
    PaDeviceIndex deviceIndex;
    const PaDeviceInfo *deviceInfo;

    SRC_STATE* srcState;
    SRC_DATA srcData;

    std::condition_variable cv;
    mutable std::mutex mtx;
    //*** Begin: mutex-protected vars ***//
    // mixed, converted to float and resampled buffer consumed by callback
    std::vector<unsigned char> interleavedProcessedBuffer;
    // false: buffer has been consumed by jack and needs to be refilled
    // true: buffer is filled
    bool ready = false;
    // true on this->start(), false on this->stop()
    bool isRunning = false;
    //*** End: mutex-protected vars ***//


    /* This routine will be called by the PortAudio engine when audio is needed.
     * It may called at interrupt level on some machines so don't do anything
     * that could mess up the system like calling malloc() or free().
     */
    static int audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
    {
        /* Cast data passed through stream to our structure. */
        PortAudioOutput *self = static_cast <PortAudioOutput*>(userData);
        float *out = (float *)outputBuffer;
        (void)inputBuffer; /* Prevent unused variable warning. */

        unsigned char outChan = self->GetOutputChannels();

        std::unique_lock<std::mutex> lck(self->d->mtx, std::defer_lock);
        if (!lck.try_lock())
        {
            CLOG(LogLevel_t::Warning, "acquiring lock failed, discarding");
            //         ret = -1;
            goto fail;
        }

        if (!self->d->isRunning)
        {
            //         ret = 0; // no error
            goto fail;
        }

        if (!self->d->ready)
        {
            CLOG(LogLevel_t::Warning, "buffer was not ready for pa, discarding");

            //         ret = -1;
            goto fail;
        }
        
        float *from = reinterpret_cast<float*>(self->d->interleavedProcessedBuffer.data());
        for (unsigned long i = 0; i < framesPerBuffer; i++)
        {
            for (unsigned char c = 0; c < outChan; c++)
            {
                *out++ = from[i * outChan + c];
            }
        }

        self->d->ready = false;
        lck.unlock();
        self->d->cv.notify_all();

        return paContinue;

    fail:
        std::memset(out, 0, framesPerBuffer * outChan * sizeof(float));
        return paContinue;
    }
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

    std::unique_lock<std::mutex> lck(d->mtx);

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

    this->d->interleavedProcessedBuffer.resize(gConfig.FramesToRender * chan * sizeof(float));

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

            // zero out any buffer in resampler, to avoid hearable cracks, when switching from one song to another
            src_reset(d->srcState);
            src_set_ratio(d->srcState, (double)d->deviceInfo->defaultSampleRate / this->currentFormat.SampleRate);
        }
    }

    // finally update channelcount, srate and sformat
    this->currentFormat = format;
}

void PortAudioOutput::_init(SongFormat &format, bool)
{
    int channels = this->GetOutputChannels();

    this->processedBuffer.clear();
    this->processedBuffer.resize(gConfig.FramesToRender * this->GetOutputChannels() * sizeof(float));

    this->drop();
    this->close();

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
                        PortAudioOutput::Impl::audioCallback,
                        this);
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

    std::unique_lock<std::mutex> lck(d->mtx);

    // wait until portaudio buffer has been consumed
    d->cv.wait(lck, [this] { return !d->ready || !d->isRunning; });

    if (d->deviceInfo->defaultSampleRate == this->currentFormat.SampleRate)
    {
        d->interleavedProcessedBuffer.swap(this->processedBuffer);
        d->ready = true;
        lck.unlock();
        // notify jacks process thread. in fact doesnt do anything, because jacks process thread doesnt this->cv.wait()
        d->cv.notify_one();
        return frames;
    }

    PaError err;
    int framesUsed = 0;
    int framesUsedNow;
    do
    {
        framesUsedNow = this->doResampling(procBuf + framesUsed * this->GetOutputChannels(), frames);
        frames -= framesUsedNow;
        framesUsed += framesUsedNow;
        
        if (d->srcData.output_frames_gen == d->srcData.output_frames)
        {
            d->srcData.output_frames_gen = 0;
            d->ready = true;
            lck.unlock();
            // notify jacks process thread. in fact doesnt do anything, because jacks process thread doesnt this->cv.wait()
            d->cv.notify_one();
            std::this_thread::yield();
            if (frames != 0)
            {
                lck.lock();
            }
        }
    } while (frames > 0 && d->isRunning);

    return framesUsed;
}

int PortAudioOutput::doResampling(const float *inBuf, const size_t Frames)
{
    if (!d->isRunning)
    {
        return 0;
    }
    if (!this->currentFormat.IsValid())
    {
        THROW_RUNTIME_ERROR("SongFormat not valid")
    }

    d->srcData.data_in = inBuf;
    d->srcData.input_frames = Frames;

    d->srcData.data_out = reinterpret_cast<float*>(d->interleavedProcessedBuffer.data());
    d->srcData.data_out += d->srcData.output_frames_gen * this->GetOutputChannels();

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
        if (d->srcData.input_frames_used < Frames)
        {
            CLOG(LogLevel_t::Info, "Not all input frames used!"
                                   << "input_frames: " << d->srcData.input_frames << "\toutput_frames: " << d->srcData.output_frames << std::endl
                                   << "input_frames_used: " << d->srcData.input_frames_used << "\toutput_frames_gen: " << d->srcData.output_frames_gen);
        }

        if (d->srcData.output_frames_gen < d->srcData.output_frames)
        {
            CLOG(LogLevel_t::Info, "resample buffer has not been filled completely" << std::endl
                                << "input_frames: " << d->srcData.input_frames << "\toutput_frames: " << d->srcData.output_frames << std::endl
                                << "input_frames_used: " << d->srcData.input_frames_used << "\toutput_frames_gen: " << d->srcData.output_frames_gen);

            // needed next time to advance data_out
            d->srcData.output_frames_gen += old;
        }
    }

    return d->srcData.input_frames_used;
}

void PortAudioOutput::start()
{
    std::unique_lock<std::mutex> lck(d->mtx);

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

    d->isRunning = true;
    lck.unlock();
    d->cv.notify_all();
}

void PortAudioOutput::stop()
{
    std::unique_lock<std::mutex> lck(d->mtx);

    d->isRunning = false;
    if (d->handle != nullptr)
    {
        // dont call Pa_StopStream() here since it causes draining the pcm, which takes time and may cause deadlocks
        // use Pa_AbortStream() instead which drops any PCM currently played
        PaError err = Pa_AbortStream(d->handle);
        if (err != PaErrorCode::paNoError && err != PaErrorCode::paStreamIsStopped)
        {
            CLOG(LogLevel_t::Error, "unable to stop pcm (" << Pa_GetErrorText(err) << ")");
        }
    }
    lck.unlock();
    d->cv.notify_all();
}
