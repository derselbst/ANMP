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

WaveOutput::WaveOutput(Player* player):player(player)
{
}

WaveOutput::~WaveOutput()
{
    this->close();
}

//
// Interface Implementation
//
void WaveOutput::open()
{
    if(Config::RenderWholeSong)
    {
        // writing the file might be done with one call to this->write(), but this doesnt mean that the song already has been fully rendered yet
        THROW_RUNTIME_ERROR("You MUST NOT hold the whole audio file in memory, when using WaveOutput.")
    }
}

void WaveOutput::init(SongFormat format, bool)
{
    this->close();
    
    this->currentSong = this->player->getCurrentSong();
    
    if(this->currentSong == nullptr || !format.IsValid())
    {
        return;
    }

    string outFile = this->currentSong->Filename.c_str();
    outFile += ".wav";
    
    this->handle = fopen(outFile.c_str(), "wb");
    if(this->handle == nullptr)
    {
        THROW_RUNTIME_ERROR("failed opening \"" << outFile << "\" for writing")
    }

    fseek(this->handle, sizeof(WaveHeader), SEEK_SET);
    
    this->currentFormat = format;
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

int WaveOutput::write (const float* buffer, frame_t frames)
{
    int ret = fwrite(buffer, sizeof(float), frames * this->currentFormat.Channels, this->handle);
    ret /= this->currentFormat.Channels;

    this->framesWritten += ret;
    return ret;
}

int WaveOutput::write (const int16_t* buffer, frame_t frames)
{
    int ret = fwrite(buffer, sizeof(int16_t), frames * this->currentFormat.Channels, this->handle);
    ret /= this->currentFormat.Channels;

    this->framesWritten += ret;
    return ret;
}

int WaveOutput::write (const int32_t* buffer, frame_t frames)
{
    int ret = fwrite(buffer, sizeof(int32_t), frames * this->currentFormat.Channels, this->handle);
    ret /= this->currentFormat.Channels;


    this->framesWritten += ret;
    return ret;
}

void WaveOutput::start()
{

}

void WaveOutput::stop()
{

}

