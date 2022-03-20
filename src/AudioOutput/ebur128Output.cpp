#include "ebur128Output.h"
#include "Config.h"
#include "Player.h"
#include "Song.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "LoudnessFile.h"

#include <iostream>
#include <string>
#include <utility> // std::pair

#include <ebur128.h>

struct ebur128Output::Impl
{
    // because this.stop() might be called concurrently to this.write()
    mutable std::recursive_mutex mtx;

    Player *player = nullptr;
    ebur128_state *handle = nullptr;

    const Song *currentSong = nullptr;
};

ebur128Output::ebur128Output(Player *p)
: d(std::make_unique<Impl>())
{
    d->player = p;
    d->player->onCurrentSongChanged += std::make_pair(this, ebur128Output::onCurrentSongChanged);
}

ebur128Output::~ebur128Output()
{
    this->close();
    d->player->onCurrentSongChanged -= this;
}

//
// Interface Implementation
//
void ebur128Output::open()
{
    std::lock_guard<std::recursive_mutex> lck(d->mtx);

    if (gConfig.RenderWholeSong && gConfig.PreRenderTime != 0)
    {
        THROW_RUNTIME_ERROR("You MUST NOT hold the whole audio file in memory, when using ebur128Output, while gConfig.PreRenderTime!=0")
    }

    if (gConfig.useAudioNormalization)
    {
        THROW_RUNTIME_ERROR("You MUST DISABLE audio normalization when generating normalization data")
    }

    if (gConfig.useLoopInfo)
    {
        THROW_RUNTIME_ERROR("You MUST NOT use loop info when generating normalization data")
    }
}

void ebur128Output::init(SongFormat &format, bool)
{
    this->currentFormat = format;
}

void ebur128Output::onCurrentSongChanged(void *context, const Song *newSong)
{
    ebur128Output *pthis = static_cast<ebur128Output *>(context);

    std::lock_guard<std::recursive_mutex> lck(pthis->d->mtx);
    try
    {
        pthis->close();

        if (newSong != nullptr)
        {
            if (!pthis->currentFormat.IsValid())
            {
                CLOG(LogLevel_t::Warning, "attempting to use invalid SongFormat");
            }

            const uint32_t chan = pthis->currentFormat.Channels();
            pthis->d->handle = ebur128_init(chan, pthis->currentFormat.SampleRate, EBUR128_MODE_TRUE_PEAK);
            if (pthis->d->handle == nullptr)
            {
                THROW_RUNTIME_ERROR("ebur128_init failed")
            }

            // set channel map (note: see ebur128.h for the default map)
            if (chan == 3)
            {
                ebur128_set_channel(pthis->d->handle, 0, EBUR128_LEFT);
                ebur128_set_channel(pthis->d->handle, 1, EBUR128_RIGHT);
                ebur128_set_channel(pthis->d->handle, 2, EBUR128_UNUSED);
            }
            else if (chan == 5)
            {
                ebur128_set_channel(pthis->d->handle, 0, EBUR128_LEFT);
                ebur128_set_channel(pthis->d->handle, 1, EBUR128_RIGHT);
                ebur128_set_channel(pthis->d->handle, 2, EBUR128_CENTER);
                ebur128_set_channel(pthis->d->handle, 3, EBUR128_LEFT_SURROUND);
                ebur128_set_channel(pthis->d->handle, 4, EBUR128_RIGHT_SURROUND);
            }
            else
            {
                // default channel map should be fine here
            }
        }
    }
    catch (...)
    {
        pthis->d->currentSong = nullptr;
        throw;
    }

    pthis->d->currentSong = newSong;
}

void ebur128Output::close()
{
    std::lock_guard<std::recursive_mutex> lck(d->mtx);

    if (d->handle != nullptr)
    {
        double overallSamplePeak = 0.0;
        for (unsigned int c = 0; c < d->handle->channels; c++)
        {
            double peak = -0.0;
            if (ebur128_true_peak(d->handle, c, &peak) == EBUR128_SUCCESS)
            {
                overallSamplePeak = std::max(peak, overallSamplePeak);
            }
        }

        float gainCorrection = overallSamplePeak;
        if (gainCorrection <= 0.0)
        {
            CLOG(LogLevel_t::Error, "ignoring gainCorrection == " << gainCorrection);
        }
        else
        {
            if (gainCorrection > 1.0)
            {
                CLOG(LogLevel_t::Info, mybasename(d->currentSong->Filename) << " gainCorrection == " << gainCorrection);
            }

            // write the collected loudness info
            LoudnessFile::write(d->currentSong->Filename, gainCorrection);
        }

        ebur128_destroy(&d->handle);
        d->handle = nullptr;
    }

    d->currentSong = nullptr;
}

int ebur128Output::write(const float *buffer, frame_t frames)
{
    std::lock_guard<std::recursive_mutex> lck(d->mtx);

    if (d->handle == nullptr)
    {
        return 0;
    }

    int ret = ebur128_add_frames_float(d->handle, buffer, frames);

    if (ret == EBUR128_SUCCESS)
    {
        return frames;
    }

    THROW_RUNTIME_ERROR("ebur128: out of memory or something else");
}

int ebur128Output::write(const int16_t *buffer, frame_t frames)
{
    std::lock_guard<std::recursive_mutex> lck(d->mtx);

    if (d->handle == nullptr)
    {
        return 0;
    }

    int ret = ~EBUR128_SUCCESS;

    if (sizeof(short) == sizeof(int16_t))
    {
        ret = ebur128_add_frames_short(d->handle, reinterpret_cast<const short *>(buffer), frames);
    }
    else if (sizeof(int) == sizeof(int16_t))
    {
        ret = ebur128_add_frames_int(d->handle, reinterpret_cast<const int *>(buffer), frames);
    }

    if (ret == EBUR128_SUCCESS)
    {
        return frames;
    }

    THROW_RUNTIME_ERROR("ebur128: out of memory or something else");
}

int ebur128Output::write(const int32_t *buffer, frame_t frames)
{
    std::lock_guard<std::recursive_mutex> lck(d->mtx);

    if (d->handle == nullptr)
    {
        return 0;
    }

    int ret = ~EBUR128_SUCCESS;

    if (sizeof(short) == sizeof(int32_t))
    {
        ret = ebur128_add_frames_short(d->handle, reinterpret_cast<const short *>(buffer), frames);
    }
    else if (sizeof(int) == sizeof(int32_t))
    {
        ret = ebur128_add_frames_int(d->handle, reinterpret_cast<const int *>(buffer), frames);
    }

    if (ret == EBUR128_SUCCESS)
    {
        return frames;
    }

    THROW_RUNTIME_ERROR("ebur128: out of memory or something else");
}

void ebur128Output::start()
{
}

void ebur128Output::stop()
{
    this->close();
}
