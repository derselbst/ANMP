#ifndef JACKOUTPUT_H
#define JACKOUTPUT_H

#include "IAudioOutput.h"

#include <mutex>
#include <vector>
#include <jack/jack.h>
#include <samplerate.h>


/**
  * class JackOutput
  *
  * A wrapper library for Jack, enabling ANMP to use Jack for playback
  */
class JackOutput : public IAudioOutput
{
public:

    JackOutput ();

    // forbid copying
    JackOutput(JackOutput const&) = delete;
    JackOutput& operator=(JackOutput const&) = delete;

    virtual ~JackOutput ();


    // interface methods declaration

    void open () override;

    void init (SongFormat format, bool realtime = false) override;

    void close () override;

    int write (const float* buffer, frame_t frames) override;

    int write (const int16_t* buffer, frame_t frames) override;

    int write (const int32_t* buffer, frame_t frames) override;

    void start () override;
    void stop () override;
    
    void SetOutputChannels(Nullable<uint16_t>) override;

protected:

    vector<jack_port_t*> playbackPorts;

    /*not static, pointer can be reassigned by jack if name is not unique*/
    const char* ClientName = "ANMP";

    jack_client_t* handle = nullptr;
    SRC_STATE *srcState = nullptr;

    typedef struct
    {
        // buffer where to store the pcm to be played by jack
        // length of this buffer determined by this->jackBufSize
        jack_default_audio_sample_t* buf = nullptr;
        
        // false: buffer has been consumed by jack and needs to be refilled
        // true: buffer is filled
        bool ready = false;
        
        bool isRunning = false;
    } jack_buffer_t;

    SRC_DATA srcData;
    mutable std::recursive_mutex mtx;
    //*** Begin: mutex-protected vars ***//
    jack_buffer_t interleavedProcessedBuffer;
    jack_nframes_t jackBufSize = 0;
    jack_nframes_t jackSampleRate = 0;
    //*** End: mutex-protected vars ***//

    static int processCallback(jack_nframes_t nframes, void* arg);
    static int onJackSampleRateChanged(jack_nframes_t nframes, void* arg);
    static void onJackShutdown(void* arg);
    static int onJackBufSizeChanged(jack_nframes_t nframes, void *arg);

    template<typename T> int write(const T* buffer, frame_t frames);
    virtual int doResampling(const float* inBuf, const size_t Frames);
    void connectPorts();
};

#endif // JACKOUTPUT_H
