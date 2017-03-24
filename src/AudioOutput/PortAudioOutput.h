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

    void init (SongFormat format, bool realtime = false) override;

    void close () override;

    void start () override;
    void stop () override;

protected:
    template<typename T> int write(const T* buffer, frame_t frames) override;
    
private:

    PaStream *handle = nullptr;

    // holds the error returned by Pa_Initialize
    // this class shall only be usable if no error occurred
    PaError paInitError = ~PaErrorCode::paNoError;

    void drain ();
    void drop ();

};

#endif // PortAudioOutput_H
