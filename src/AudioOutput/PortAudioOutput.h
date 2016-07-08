#ifndef PortAudioOutput_H
#define PortAudioOutput_H

#include "IAudioOutput.h"

#include <portaudio.h>


/**
  * class PortAudioOutput
  *
  * A wrapper library for portaudio, enabling ANMP for crossplatform playback
  */
class PortAudioOutput : public IAudioOutput
{
public:

    PortAudioOutput ();

    // forbid copying
    PortAudioOutput(PortAudioOutput const&) = delete;
    PortAudioOutput& operator=(PortAudioOutput const&) = delete;
    
    virtual ~PortAudioOutput ();


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

	PaStream *handle = nullptr;
	
	// holds the error returned by Pa_Initialize
	// this class shall only be usable if no error occurred
	PaError paInitError = ~PaErrorCode::paNoError
    
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

#endif // PortAudioOutput_H
