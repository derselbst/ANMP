#ifndef JACKOUTPUT_H
#define JACKOUTPUT_H

#include "IAudioOutput.h"

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
  
  /*not static!*/ const char* ClientName = "ANMP";
  jack_client_t* handles = nullptr;
  
  static int processCallback(jack_nframes_t nframes, void* arg);
  static int onJackSampleRateChanged(jack_nframes_t nframes, void* arg);
  static void onJackShutdown(void* arg);
};

#endif // JACKOUTPUT_H
