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

    int write (const float* buffer, frame_t frames) override;

    int write (const int16_t* buffer, frame_t frames) override;

    int write (const int32_t* buffer, frame_t frames) override;

    void start () override;
    void stop () override;

private:

    PaStream *handle = nullptr;

    // holds the error returned by Pa_Initialize
    // this class shall only be usable if no error occurred
    PaError paInitError = ~PaErrorCode::paNoError;

    template<typename T> int write(const T* buffer, frame_t frames);
    void drain ();
    void drop ();

};

#endif // PortAudioOutput_H
