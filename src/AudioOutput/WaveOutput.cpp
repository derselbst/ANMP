#include "WaveOutput.h"
#include "Player.h"
#include "StandardWrapper.h"

#include "LoudnessFile.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <iostream>
#include <string>
#include <utility>      // std::pair
#include <cstring>

void WaveOutput::SongChanged(void* ctx)
{
    WaveOutput* context = static_cast<WaveOutput*>(ctx);
    
    context->close();
    context->currentSong = context->player->getCurrentSong();
    if(context->currentSong == nullptr)
    {
        return;
    }
    
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof (sfinfo));
    sfinfo.samplerate = context->currentSampleRate;
    sfinfo.channels = context->currentChannelCount;
    sfinfo.format = SF_FORMAT_WAV;
    switch(context->currentSampleFormat)
    {
      case int16:
	sfinfo.format |= SF_FORMAT_PCM_16;
	break;
      case int32:
	sfinfo.format |= SF_FORMAT_PCM_32;
	break;
      case float32:
	sfinfo.format |= SF_FORMAT_FLOAT;
	break;
      default:
	sfinfo.format |= SF_FORMAT_PCM_16;
	break;
    }
    
    string outFile = context->currentSong->Filename.c_str();
    outFile += ".wav";
    context->handle = sf_open(outFile.c_str(), SFM_WRITE, &sfinfo);
}

WaveOutput::WaveOutput(Player* player):player(player)
{
    this->player->onCurrentSongChanged += std::make_pair(this, &WaveOutput::SongChanged);
}

WaveOutput::~WaveOutput()
{
    this->close();
    this->player->onCurrentSongChanged -= this;
}

//
// Interface Implementation
//
void WaveOutput::open()
{
        if(Config::RenderWholeSong)
        {
           THROW_RUNTIME_ERROR("You MUST NOT hold the whole audio file in memory, when using WaveOutput.")
        }
}

void WaveOutput::init(unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime)
{
    this->currentChannelCount = channels;
    this->currentSampleFormat = s;
    this->currentSampleRate = sampleRate;
}

void WaveOutput::close()
{
    if (this->handle != nullptr)
    {
        sf_close(this->handle);
        this->handle = nullptr;
    }
    
    this->currentSong = nullptr;
    this->framesWritten = 0;
}

int WaveOutput::write (float* buffer, frame_t frames)
{
    int ret = sf_writef_float(this->handle, buffer, frames);
    
        this->framesWritten += ret;
        return ret;
}

int WaveOutput::write (int16_t* buffer, frame_t frames)
{
    int ret;
    
    if(sizeof(short)==sizeof(int16_t))
    {
        ret = sf_writef_short(this->handle, reinterpret_cast<short*>(buffer), frames);
    }
    else if(sizeof(int)==sizeof(int16_t))
    {
        ret = sf_writef_int(this->handle, reinterpret_cast<int*>(buffer), frames);
    }
    
        this->framesWritten += ret;
        return ret;
}

int WaveOutput::write (int32_t* buffer, frame_t frames)
{
    int ret;
    
    if(sizeof(short)==sizeof(int32_t))
    {
        ret = sf_writef_short(this->handle, reinterpret_cast<short*>(buffer), frames);
    }
    else if(sizeof(int)==sizeof(int32_t))
    {
        ret = sf_writef_int(this->handle, reinterpret_cast<int*>(buffer), frames);
    }
    
        this->framesWritten += ret;
        return ret;
}

void WaveOutput::setVolume(uint8_t vol)
{
    
}

// Accessor methods
//


// Other methods
//


void WaveOutput::start()
{
    
}

void WaveOutput::stop()
{
    
}

