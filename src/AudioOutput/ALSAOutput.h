#ifndef ALSAOUTPUT_H
#define ALSAOUTPUT_H

#include <cstdint>

#include "IAudioOutput.h"



#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>
#include <sys/time.h>

#if defined (__linux__)
#include     <fcntl.h>
#include     <sys/ioctl.h>
#include     <sys/soundcard.h>
#endif




/**
  * class ALSAOutput
  *
  */

class ALSAOutput : public IAudioOutput
{
public:

    // Constructors/Destructors
    //


    /**
     * Empty Constructor
     */
    ALSAOutput ();

    /**
     * Empty Destructor
     */
    virtual ~ALSAOutput ();


    // interface methods declaration

    void open () override;

    void init (unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime = false) override;

    void drain () override;

    void drop () override;

    void close () override;

    int write (float* buffer, unsigned int frames) override;

    int write (int16_t* buffer, unsigned int frames) override;

    void start () override;
private:


    snd_pcm_t *alsa_dev = nullptr;
};

#endif // ALSAOUTPUT_H
