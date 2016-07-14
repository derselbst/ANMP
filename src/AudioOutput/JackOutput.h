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

    void init (unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime = false) override;

    void close () override;

    int write (float* buffer, frame_t frames) override;

    int write (int16_t* buffer, frame_t frames) override;

    int write (int32_t* buffer, frame_t frames) override;

    void start () override;
    void stop () override;

private:

  vector<jack_port_t*> playbackPorts;
  
  /*not static, pointer can be reassigned by jack if name is not unique*/
  const char* ClientName = "ANMP";
  
  jack_client_t* handle = nullptr;
  SRC_STATE *srcState = nullptr;
  
  typedef struct
  {
      jack_default_audio_sample_t* buf = nullptr; // length of this buffer determined by jackBufSize
      volatile bool ready = false;
  } jack_buffer_t;
  
  mutable std::mutex mtx;
  //*** Begin: mutex-protected vars ***//
  jack_buffer_t interleavedProcessedBuffer;
  volatile jack_nframes_t jackBufSize = 0;
  volatile jack_nframes_t jackSampleRate = 0;
  jack_transport_state_t transportState = JackTransportStopped;
  //*** End: mutex-protected vars ***//
  
  static int processCallback(jack_nframes_t nframes, void* arg);
  static int onJackSampleRateChanged(jack_nframes_t nframes, void* arg);
  static void onJackShutdown(void* arg);
  static int onJackBufSizeChanged(jack_nframes_t nframes, void *arg);
  
  template<typename T> int write(T* buffer, frame_t frames);
  template<typename T> void getAmplifiedFloatBuffer(const T* inBuf, float* outBuf, const size_t Items);
  int doResampling(float* inBuf, const size_t Frames);
  void connectPorts();
};

#endif // JACKOUTPUT_H
