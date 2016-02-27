#ifndef IAUDIOOUTPUT_H
#define IAUDIOOUTPUT_H

#include <cstdint>

#include "types.h"

using namespace std;

/**
  * class IAudioOutput
  *
  */

class IAudioOutput
{
protected:
    // this is an interface, even if there were no pure virtual methods, allow
    // construction for child classes only
    IAudioOutput();

public:

    IAudioOutput(IAudioOutput const&) = delete;
    IAudioOutput& operator=(IAudioOutput const&) = delete;

    // virtual destructor for proper cleanup
    virtual ~IAudioOutput();

    /**
     * opens a sound device, e.g. set buffer size, periods, pcm format (int16,
     * float,...), i.e. things that dont change while running program
     *
     * shall only be called once
     */
    virtual void open () = 0;


    /**
     * initializes a sound device, e.g. set samplerate, channels, i.e. settings that
     * can change while running program
     *
     * can be called multiple time if necessary
     */
    virtual void init (unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime=false) = 0;


    /**
     * Wait for all pending frames to be played and then stop the PCM.
     */
    virtual void start () = 0;


    /**
     * Wait for all pending frames to be played and then stop the PCM.
     */
    virtual void drain () = 0;


    /**
     * This function stops the PCM immediately. The pending samples on the buffer are
     * ignored.
     */
    virtual void drop () = 0;


    /**
     * closes the device
     */
    virtual void close () = 0;


    /**
     * @param  buffer buffer that holds the pcm
     * @param  frames no. of frames to be played from the buffer
     * @param  channels no. of channels
     */
    virtual int write (float* buffer, unsigned int frames) = 0;


    /**
     * @param  buffer
     * @param  frames
     * @param  channels
     */
    virtual int write (int16_t* buffer, unsigned int frames) = 0;


    /**
     * @param  frameBuffer
     * @param  frames frames to play
     * @param  channels
     * @param  offset item offset within frameBuffer, i.e. how many items (determined
     * by pcmFormat) to skip; this parameter shall avoid the caller to take care of
     * pointer arithmetic
     * @param  pcmFormat specifies the format for the items frameBuffer (int16, float,
     * etc.)
     */
    virtual int write (pcm_t* frameBuffer, unsigned int frames, int offset);

protected:
    unsigned char currentChannelCount = 0;
    SampleFormat_t currentSampleFormat = SampleFormat_t::unknown;

};

#endif // IAUDIOOUTPUT_H
