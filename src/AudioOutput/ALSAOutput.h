#ifndef ALSAOUTPUT_H
#define ALSAOUTPUT_H

#include <cstdint>

#include "IAudioOutput.h"


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

    void open (bool realtime = false) override;

    void init (unsigned int sampleRate, uint8_t channels, SampleFormat_t s) override;

    void drain () override;

    void drop () override;

    void close () override;

    void write (float* buffer, unsigned int frames) override;

    void write (int16_t* buffer, unsigned int frames) override;

    void write (pcm_t* frameBuffer, unsigned int frames, int offset, SampleFormat_t pcmFormat) override;

    
private:
  
  void start ();
  snd_pcm_t *alsa_dev = nullptr;
  unsigned char lastChannelCount = 0;
};

#endif // ALSAOUTPUT_H
