#ifndef ALSAOUTPUT_H
#define ALSAOUTPUT_H

#include "IAudioOutput.h"

// just forward declare this type by ourself; it's actually
// declared in alsa/asoundlib.h; but it's a complete overkill
// to include ALSA in this header
typedef struct _snd_pcm snd_pcm_t;


/**
  * class ALSAOutput
  *
  * A wrapper library for ALSA, enabling ANMP to use ALSA for playback
  */
class ALSAOutput : public IAudioOutput
{
public:

    ALSAOutput ();

    // forbid copying
    ALSAOutput(ALSAOutput const&) = delete;
    ALSAOutput& operator=(ALSAOutput const&) = delete;
    
    virtual ~ALSAOutput ();


    // interface methods declaration

    void open () override;

    void init (unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime = false) override;

    void close () override;

    int write (float* buffer, frame_t frames) override;

    int write (int16_t* buffer, frame_t frames) override;

    int write (int32_t* buffer, frame_t frames) override;

    void setVolume(float) override;

    void start () override;
    void stop () override;

private:

    int epipe_count = 0;
    snd_pcm_t* alsa_dev = nullptr;
    
    // the current volume [0,1.0] to use
    // mark this as volatile so the compiler doesnt come up with:
    // "oh, this member isnt modified in the current scope. lets put it to a register, while using this var inside a loop again and again."
    // volatile here hopefully forces that each read actually happens through memory (I dont worry too much about hardware caching here)
    // NOTE: I dont need thread safety for this variable and I know that volatile doesnt do anything for that either. its just because
    // one thread will always read from this var (which happens in this->write(T*, frame_t)) and another thread occasionally comes along and alters
    // this var (inside this->setVolume())
    // the worst things that can happen here are dirty reads, as far as I see; and who cares?
    // however, Im not absolutely sure if volatile if really required here
    volatile float volume = 1.0f;

    template<typename T> int write(T* buffer, frame_t frames);
    void drain ();
    void drop ();

};

#endif // ALSAOUTPUT_H
