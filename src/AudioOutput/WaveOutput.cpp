#include "WaveOutput.h"
#include "Player.h"
#include "Song.h"
#include "Config.h"
#include "Common.h"

#include "LoudnessFile.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <iostream>
#include <string>
#include <utility>      // std::pair
#include <cstring>
#include <errno.h>

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
    lock_guard<mutex> lck(this->mtx);
    
    if(gConfig.RenderWholeSong && gConfig.PreRenderTime!=0)
    {
        // writing the file might be done with one call to this->write(), but this doesnt mean that the song already has been fully rendered yet
        THROW_RUNTIME_ERROR("You MUST NOT hold the whole audio file in memory, when using WaveOutput, while gConfig.PreRenderTime!=0")
    }
}

void WaveOutput::init(SongFormat format, bool)
{
    this->close();
    
    lock_guard<mutex> lck(this->mtx);
    
    this->currentSong = this->player->getCurrentSong();
    
    if(this->currentSong == nullptr || !format.IsValid())
    {
        return;
    }

    string outFile = ::getUniqueFilename(this->currentSong->Filename + ".wav");
    
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
    lock_guard<mutex> lck(this->mtx);
    
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


template<typename T> int WaveOutput::write(const T* buffer, frame_t frames)
{
    int ret=0;
    
    lock_guard<mutex> lck(this->mtx);
    
    if(this->handle!=nullptr)
    {
        ret = fwrite(buffer, sizeof(T), frames * this->currentFormat.Channels, this->handle);
        ret /= this->currentFormat.Channels;
        
        if(ret != frames)
        {
            THROW_RUNTIME_ERROR("fwrite failed writing " << frames << " frames, errno says: " << strerror(errno))
        }

        this->framesWritten += ret;
    }
    
    return ret;
}

int WaveOutput::write (const float* buffer, frame_t frames)
{    
    return this->write<float>(buffer, frames);
}

int WaveOutput::write (const int16_t* buffer, frame_t frames)
{    
    return this->write<int16_t>(buffer, frames);
}

int WaveOutput::write (const int32_t* buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
}

void WaveOutput::start()
{

}

void WaveOutput::stop()
{
    this->close();
}

