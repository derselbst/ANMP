#pragma once

#include "IAudioOutput.h"
#include "Song.h"
#include "CommonExceptions.h"
#include <cstdio>
#include <mutex>

class Player;

/**
  * class WaveOutput
  *
  * A wrapper library for ALSA, enabling ANMP to use ALSA for playback
  */
class WaveOutput : public IAudioOutput
{
public:

    WaveOutput (Player*);

    // forbid copying
    WaveOutput(WaveOutput const&) = delete;
    WaveOutput& operator=(WaveOutput const&) = delete;

    virtual ~WaveOutput ();

    // interface methods declaration

    void open () override;

    void init (SongFormat format, bool realtime = false) override;

    void close () override;

    void start () override;
    void stop () override;

protected:
    template<typename T> int write(const T* buffer, frame_t frames) override;
    
private:
    // because this.stop() might be called concurrently to this.write()
    mutable recursive_mutex mtx;
    
    Player* player = nullptr;
    FILE* handle = nullptr;

    const Song* currentSong = nullptr;
    frame_t framesWritten = 0;

    static void onCurrentSongChanged(void* ctx);
};
