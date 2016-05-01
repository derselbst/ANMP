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

    virtual ~ALSAOutput ();


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

    int epipe_count = 0;
    snd_pcm_t* alsa_dev = nullptr;

    template<typename T> int write(T* buffer, frame_t frames);
    void drain ();
    void drop ();

};

#endif // ALSAOUTPUT_H
