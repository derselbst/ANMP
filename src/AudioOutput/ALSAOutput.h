#ifndef ALSAOUTPUT_H
#define ALSAOUTPUT_H

#include <cstdint>

#include "IAudioOutput.h"



#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
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



    void close () override;

    int write (float* buffer, frame_t frames) override;

    int write (int16_t* buffer, frame_t frames) override;
    
    void setVolume(uint8_t) override;

    void start () override;
    void stop () override;
private:

    snd_pcm_t *alsa_dev = nullptr;
    
    void drain ();

    void drop ();
    
};

#endif // ALSAOUTPUT_H
