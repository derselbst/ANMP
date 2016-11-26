#include "ebur128Output.h"
#include "Player.h"
#include "Song.h"
#include "Config.h"

#include "LoudnessFile.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <iostream>
#include <string>
#include <utility>      // std::pair

void ebur128Output::SongChanged(void* ctx)
{
    ebur128Output* context = static_cast<ebur128Output*>(ctx);

    context->close();

    context->currentSong = context->player->getCurrentSong();
    if(context->currentSong == nullptr)
    {
        return;
    }

    context->handle = ebur128_init(context->currentChannelCount, context->currentSampleRate, EBUR128_MODE_TRUE_PEAK);

    // set channel map (note: see ebur128.h for the default map)
    if (context->currentChannelCount == 3)
    {
        ebur128_set_channel(context->handle, 0, EBUR128_LEFT);
        ebur128_set_channel(context->handle, 1, EBUR128_RIGHT);
        ebur128_set_channel(context->handle, 2, EBUR128_UNUSED);
    }
    else if (context->currentChannelCount == 5)
    {
        ebur128_set_channel(context->handle, 0, EBUR128_LEFT);
        ebur128_set_channel(context->handle, 1, EBUR128_RIGHT);
        ebur128_set_channel(context->handle, 2, EBUR128_CENTER);
        ebur128_set_channel(context->handle, 3, EBUR128_LEFT_SURROUND);
        ebur128_set_channel(context->handle, 4, EBUR128_RIGHT_SURROUND);
    }
    else
    {
        // default channel map should be fine here
    }
}

ebur128Output::ebur128Output(Player* player):player(player)
{
    this->player->onCurrentSongChanged += std::make_pair(this, &ebur128Output::SongChanged);
}

ebur128Output::~ebur128Output()
{
    this->close();
    this->player->onCurrentSongChanged -= this;
}

//
// Interface Implementation
//
void ebur128Output::open()
{
    if(Config::RenderWholeSong)
    {
        THROW_RUNTIME_ERROR("You MUST NOT hold the whole audio file in memory, when using ebur128Output.")
    }

    if(Config::useAudioNormalization)
    {
        THROW_RUNTIME_ERROR("You MUST DISABLE audio normalization when generating normalization data")
    }
}

void ebur128Output::init(unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool)
{
    this->currentChannelCount = channels;
    this->currentSampleFormat = s;
    this->currentSampleRate = sampleRate;
}

void ebur128Output::close()
{
    if (this->handle != nullptr)
    {
        double overallSamplePeak=0.0;
        for(unsigned int c = 0; c<this->handle->channels; c++)
        {
            double peak = -0.0;
            if(ebur128_true_peak(this->handle, c, &peak) == EBUR128_SUCCESS)
            {
                overallSamplePeak = max(peak, overallSamplePeak);
            }
        }

        float gainCorrection = overallSamplePeak;
        if(gainCorrection <= 0.0)
        {
            CLOG(LogLevel::ERROR, "ignoring gainCorrection == " << gainCorrection);
        }
        else
        {
            if(gainCorrection > 1.0)
            {
                CLOG(LogLevel::INFO, "gainCorrection == " << gainCorrection);
            }
        
            // write the collected loudness info
            LoudnessFile::write(this->currentSong->Filename, gainCorrection);
        }
        
        ebur128_destroy(&this->handle);
        this->handle = nullptr;
    }

    this->currentSong = nullptr;
    this->framesWritten = 0;
}

int ebur128Output::write (const float* buffer, frame_t frames)
{
    int ret = ebur128_add_frames_float(this->handle, buffer, frames);

    if(ret == EBUR128_SUCCESS)
    {
        this->framesWritten += frames;
        return frames;
    }

    THROW_RUNTIME_ERROR("ebur128: out of memory or something else");
}

int ebur128Output::write (const int16_t* buffer, frame_t frames)
{
    int ret = ~EBUR128_SUCCESS;

    if(sizeof(short)==sizeof(int16_t))
    {
        ret = ebur128_add_frames_short(this->handle, reinterpret_cast<const short*>(buffer), frames);
    }
    else if(sizeof(int)==sizeof(int16_t))
    {
        ret = ebur128_add_frames_int(this->handle, reinterpret_cast<const int*>(buffer), frames);
    }

    if(ret == EBUR128_SUCCESS)
    {
        this->framesWritten += frames;
        return frames;
    }

    THROW_RUNTIME_ERROR("ebur128: out of memory or something else");
}

int ebur128Output::write (const int32_t* buffer, frame_t frames)
{
    int ret = ~EBUR128_SUCCESS;

    if(sizeof(short)==sizeof(int32_t))
    {
        ret = ebur128_add_frames_short(this->handle, reinterpret_cast<const short*>(buffer), frames);
    }
    else if(sizeof(int)==sizeof(int32_t))
    {
        ret = ebur128_add_frames_int(this->handle, reinterpret_cast<const int*>(buffer), frames);
    }

    if(ret == EBUR128_SUCCESS)
    {
        this->framesWritten += frames;
        return frames;
    }

    THROW_RUNTIME_ERROR("ebur128: out of memory or something else");
}

void ebur128Output::start()
{

}

void ebur128Output::stop()
{

}

