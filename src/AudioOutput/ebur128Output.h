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

    static void SongChanged(void* ctx);
    
    
    // interface methods declaration

    void open () override;

    void init (unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime = false) override;

    void close () override;

    int write (float* buffer, frame_t frames) override;

    int write (int16_t* buffer, frame_t frames) override;

    int write (int32_t* buffer, frame_t frames) override;

    void start () override;
    void stop () override;

private:
    Player* player = nullptr;
    ebur128_state* handle = nullptr;
    
    const Song* currentSong = nullptr;
    frame_t framesWritten = 0;

};
