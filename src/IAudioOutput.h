
#ifndef IAUDIOOUTPUT_H
#define IAUDIOOUTPUT_H

#include <string>

/**
  * class IAudioOutput
  * 
  */

class IAudioOutput
{
public:



    /**
     * opens a sound device, e.g. set buffer size, periods, pcm format (int16,
     * float,...), i.e. things that dont change while running program
     * 
     * shall only be called once
     */
    virtual void open ();


    /**
     * initializes a sound device, e.g. set samplerate, channels, i.e. settings that
     * can change while running program
     * 
     * can be called multiple time if necessary
     */
    virtual void init ();


    /**
     * Wait for all pending frames to be played and then stop the PCM.
     */
    virtual void drain ();


    /**
     * This function stops the PCM immediately. The pending samples on the buffer are
     * ignored.
     */
    virtual void drop ();


    /**
     * closes the device
     */
    virtual void close ();


    /**
     * @param  buffer buffer that holds the pcm
     * @param  frames no. of frames to be played from the buffer
     * @param  channels no. of channels
     */
    virtual void write (float* buffer, unsigned int frames, unsigned char channels);


    /**
     * @param  buffer
     * @param  frames
     * @param  channels
     */
    virtual void write (int16_t* buffer, unsigned int frames, unsigned char channels);


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
    virtual void write (pcm_t frameBuffer, unsigned int frames, unsigned int channels, int offset, SampleFormat_t pcmFormat);

protected:

public:

protected:

public:

protected:


private:

public:

private:

public:

private:



};

#endif // IAUDIOOUTPUT_H
