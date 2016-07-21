#ifndef IAUDIOOUTPUT_H
#define IAUDIOOUTPUT_H

#include "types.h"


#include <cstdint>


using namespace std;

/**
  * Abstract base class for all classes that handle audio playback in ANMP
  */
class IAudioOutput
{
protected:
    // this is an interface, even if there were no pure virtual methods, allow
    // construction for child classes only
    IAudioOutput();

public:

    // no copy or assignment
    IAudioOutput(IAudioOutput const&) = delete;
    IAudioOutput& operator=(IAudioOutput const&) = delete;

    // virtual destructor for proper cleanup
    virtual ~IAudioOutput();


    /**
     * opens a sound device, e.g. set buffer size, periods,... i.e. things that dont change while running program
     *
     * can and shall be called initially (i.e. after object creation). can only be called again after a call to close()
     */
    virtual void open () = 0;

    /**
     * initializes a sound device, e.g. set samplerate, channels, i.e. settings that
     * can change while running program
     *
     * can be called multiple times if necessary
     * 
     * when finishing the call to this->init() the PCM stream shall be in state "stopped"
     */
    virtual void init (unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime=false) = 0;

    /**
     * Starts the PCM stream.
     * 
     * If the stream is already started, no error shall be risen.
     */
    virtual void start () = 0;

    /**
     * Stops the PCM stream immediately, i.e. when returning from this->stop() all pcm must have been processed, favourably by dropping them at once.
     * 
     * If the stream is already stopped, no error shall be risen.
     */
    virtual void stop () = 0;

    /**
     * closes the device, frees all ressources allocated by this->open()
     * 
     * Multiple calls to this->close() without corresponding calls to this->open() shall raise no error.
     */
    virtual void close () = 0;

    /**
     * sets the playback volume
     * @param vol volume [0;100]
     */
    virtual void setVolume(float vol);

    /**
     * pushes the pcm pointed to by buffer to the underlying audio driver and by that causes it to play
     *
     * this generic version also takes care of pointer arithmetic
     *
     * @param  frameBuffer buffer that holds the pcm
     * @param  frames no. of frames to be played from the buffer
     * @param  offset item offset within frameBuffer, i.e. how many items (determined
     * by pcmFormat) to skip; this parameter shall avoid the caller to take care of
     * pointer arithmetic
     *
     * @note "frameBuffer" has to contain at least ("frames"*this->currentChannelCount) items following "offset",
     * i.e. sizeof(frameBuffer)==(frames*this->currentChannelCount*sizeof(DataTypePointedToByBuffer))
     *
     * @return number of frames successfully pushed to underlying audio driver
     * 
     * @warning you can return a number smaller "frames" (but greater 0), however this case cannot always be recovered. you should better return 0 and play nothing, if you face such a problem.
     */
    virtual int write (pcm_t* frameBuffer, frame_t frames, int offset);


protected:
    uint8_t currentChannelCount = 0;
    unsigned int currentSampleRate = 0;
    SampleFormat_t currentSampleFormat = SampleFormat_t::unknown;


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

    template<typename T> void getAmplifiedBuffer(const T* inBuffer, T* outBuffer, unsigned long items);

    /**
     * pushes the pcm pointed to by buffer to the underlying audio driver and by that causes it to play
     *
     * @param  buffer buffer that holds the pcm
     * @param  frames no. of frames to be played from the buffer
     *
     * @note buffer has to contain (frames*this->currentChannelCount) items, i.e. sizeof(buffer)==(frames*this->currentChannelCount*sizeof(DataTypePointedToByBuffer))
     *
     * @return number of frames successfully pushed to underlying audio driver
     * 
     * @warning you can return a number smaller "frames" (but greater 0), however this case cannot always be recovered. you should better return 0 and play nothing, if you face such a problem.
     */
    virtual int write (float* buffer, frame_t frames) = 0;
    virtual int write (int16_t* buffer, frame_t frames) = 0;
    virtual int write (int32_t* buffer, frame_t frames) = 0;

};

#endif // IAUDIOOUTPUT_H

#include "IAudioOutput_impl.h"

