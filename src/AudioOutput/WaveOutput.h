#pragma once

#include "IAudioOutput.h"
#include <sndfile.h>

class Player;
class Song;

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

    static void SongChanged(void* ctx);
    
    
    // interface methods declaration

    void open () override;

    void init (unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime = false) override;

    void close () override;

    int write (float* buffer, frame_t frames) override;

    int write (int16_t* buffer, frame_t frames) override;

    int write (int32_t* buffer, frame_t frames) override;

    void setVolume(uint8_t) override;

    void start () override;
    void stop () override;

private:
    Player* player = nullptr;
    SNDFILE* handle = nullptr;
    
    const Song* currentSong = nullptr;
    frame_t framesWritten = 0;

};
