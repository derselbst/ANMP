#ifndef JACKOUTPUT_H
#define JACKOUTPUT_H

#include "IAudioOutput.h"

#include <mutex>

// just forward declare this type by ourself; it's actually
// declared in alsa/asoundlib.h; but it's a complete overkill
// to include ALSA in this header
typedef struct _snd_pcm snd_pcm_t;


/**
  * class JackOutput
  *
  * A wrapper library for ALSA, enabling ANMP to use ALSA for playback
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
      bool ready = false;
      volatile bool consumed = true;
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
  
  void connectPorts();
};

#endif // JACKOUTPUT_H
