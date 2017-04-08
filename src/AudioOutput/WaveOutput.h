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
    static void onCurrentSongChanged(void* ctx);

    WaveOutput (Player*);

    // forbid copying
    WaveOutput(WaveOutput const&) = delete;
    WaveOutput& operator=(WaveOutput const&) = delete;

    virtual ~WaveOutput ();

    // interface methods declaration

    void open () override;

    void init (SongFormat& format, bool realtime = false) override;

    void close () override;

    int write (const float* buffer, frame_t frames) override;

    int write (const int16_t* buffer, frame_t frames) override;

    int write (const int32_t* buffer, frame_t frames) override;

    void start () override;
    void stop () override;

private:
    // because this.stop() might be called concurrently to this.write()
    mutable recursive_mutex mtx;
    
    Player* player = nullptr;
    FILE* handle = nullptr;

    const Song* currentSong = nullptr;
    frame_t framesWritten = 0;

    template<typename T> int write(const T* buffer, frame_t frames);
};
