#pragma once

#include "IAudioOutput.h"
#include <ebur128.h>

class Player;
class Song;

/**
  * class ebur128Output
  *
  * A wrapper library for ALSA, enabling ANMP to use ALSA for playback
  */
class ebur128Output : public IAudioOutput
{
public:

    ebur128Output (Player*);

    // forbid copying
    ebur128Output(ebur128Output const&) = delete;
    ebur128Output& operator=(ebur128Output const&) = delete;

    virtual ~ebur128Output ();

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
    ebur128_state* handle = nullptr;

    const Song* currentSong = nullptr;
    static void onCurrentSongChanged(void* ctx);
};
