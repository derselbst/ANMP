#include "WaveOutput.h"
#include "Player.h"
#include "Song.h"
#include "Config.h"

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
    
    string outFile = context->currentSong->Filename.c_str();
    outFile += ".wav";
    context->handle = fopen(outFile.c_str(), "wb");
    
    fseek(context->handle, sizeof(WaveHeader), SEEK_SET);
}

WaveOutput::WaveOutput(Player* player):player(player)
{
    this->player->onCurrentSongChanged += std::make_pair(this, &WaveOutput::SongChanged);
}

WaveOutput::~WaveOutput()
{
    this->player->onCurrentSongChanged -= this;
    this->close();
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

void WaveOutput::init(unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool)
{
    this->currentChannelCount = channels;
    this->currentSampleFormat = s;
    this->currentSampleRate = sampleRate;
}

void WaveOutput::close()
{
    if(this->handle != nullptr && this->currentSong != nullptr)
    {
        WaveHeader w(this->currentSong);
        
        fseek(this->handle, 0, SEEK_SET);
        fwrite(&w, sizeof(w), 1, this->handle);
        fclose(this->handle);
        this->handle = nullptr;
    }
    
    this->currentSong = nullptr;
    this->framesWritten = 0;
}

int WaveOutput::write (float* buffer, frame_t frames)
{
    int ret = fwrite(buffer, sizeof(float), frames * this->currentChannelCount, this->handle);
    ret /= this->currentChannelCount;
    
        this->framesWritten += ret;
        return ret;
}

int WaveOutput::write (int16_t* buffer, frame_t frames)
{
    int ret = fwrite(buffer, sizeof(int16_t), frames * this->currentChannelCount, this->handle);
    ret /= this->currentChannelCount;
    
        this->framesWritten += ret;
        return ret;
}

int WaveOutput::write (int32_t* buffer, frame_t frames)
{
    int ret = fwrite(buffer, sizeof(int32_t), frames * this->currentChannelCount, this->handle);
    ret /= this->currentChannelCount;
    
    
        this->framesWritten += ret;
        return ret;
}

void WaveOutput::setVolume(float)
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

