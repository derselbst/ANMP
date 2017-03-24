#ifndef IAUDIOOUTPUT_H
#define IAUDIOOUTPUT_H

#include "types.h"
#include "SongFormat.h"

#include <cstdint>


using namespace std;

/**
  * Abstract base class for all classes that handle audio playback in ANMP
  *
  * need to support a new Audio Playback API? --> derive this class and implement all abstract methods
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
     * opens a sound device, e.g. set buffer size, periods,... i.e. things that dont change while running ANMP
     *
     * can and shall be called initially (i.e. after object creation). can only be called again after a call to close()
     */
    virtual void open () = 0;

    /**
     * initializes a sound device, e.g. set samplerate, channels, i.e. settings that
     * can change while running ANMP
     *
     * can be called multiple times if necessary
     *
     * when finishing the call to this->init() the PCM stream shall be in state "stopped"
     */
    virtual void init (SongFormat format, bool realtime=false) = 0;

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
     * @param vol volume usually ranged [0.0,1.0]
     */
    virtual void setVolume(float vol);

    /**
     * pushes the pcm pointed to by frameBuffer to the underlying audio driver (or at least schedules it for pushing/playing)
     *
     * this generic version only takes care of pointer arithmetic, passing the call on to the specific write() methods below
     * 
     * if necessary, make a deep copy of frameBuffer, since it cannot be guaranteed that it is still alive after returning
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
    virtual int write (const pcm_t* frameBuffer, frame_t frames, int offset);


protected:
    SongFormat currentFormat;

    // the current volume [0,1.0] to use, i.e. a factor by that the PCM gets amplified.
    // mark this as volatile so the compiler doesnt come up with:
    // "oh, this member isnt modified in the current scope. lets put it to a register, while using this var inside a loop again and again."
    // volatile here hopefully forces that each read actually happens through memory (I dont worry too much about hardware caching here)
    // NOTE: I dont need thread safety for this variable and I know that volatile doesnt do anything for that either. its just because
    // one thread will always read from this var (which happens in this->write(T*, frame_t)) and another thread occasionally comes along and alters
    // this var (inside this->setVolume())
    // the worst things that can happen here are dirty reads, as far as I see; and who cares?
    // however, Im not absolutely sure if volatile if really required here
    //
    // update 2016-11-22: actually this is necessary, since it prohibits the optimizer to vectorize any loop where this var is used
    volatile float volume = 1.0f;

    /**
     * pushes the pcm pointed to by buffer to the underlying audio driver and by that causes it to play
     *
     * @param  buffer buffer that holds the pcm
     * @param  frames no. of frames to be played from the buffer
     *
     * @note buffer has to contain (frames*this->currentChannelCount) items, i.e. sizeof(*buffer)==(frames*this->currentChannelCount*sizeof(DataTypePointedToByBuffer))
     *
     * @return number of frames successfully pushed to underlying audio driver. if this is zero, nothing was played and the caller may try again, after waiting 1ms or so.
     *
     * @warning you can return a number smaller "frames" (but greater 0), however this case can only berecovered, if the whole song is hold in memory. you should better return 0 and play nothing, if you face such a problem.
     */
    template<typename T> virtual int write(const T* buffer, frame_t frames) = 0;

};

#endif // IAUDIOOUTPUT_H

#include "IAudioOutput_impl.h"

