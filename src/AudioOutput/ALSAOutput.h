#ifndef ALSAOUTPUT_H
#define ALSAOUTPUT_H

#include "IAudioOutput.h"

#include <vector>

// just forward declare this type by ourself; it's actually
// declared in alsa/asoundlib.h; but it's a complete overkill
// to include ALSA in this header
typedef struct _snd_pcm snd_pcm_t;


/**
  * class ALSAOutput
  *
  * A wrapper library for ALSA, enabling ANMP to use ALSA for playback
  */
class ALSAOutput : public IAudioOutput
{
    public:
    ALSAOutput();

    // forbid copying
    ALSAOutput(ALSAOutput const &) = delete;
    ALSAOutput &operator=(ALSAOutput const &) = delete;

    virtual ~ALSAOutput();


    // interface methods declaration

    void open() override;

    void init(SongFormat &format, bool realtime = false) override;

    void close() override;

    int write(const float *buffer, frame_t frames) override;

    int write(const int16_t *buffer, frame_t frames) override;

    int write(const int32_t *buffer, frame_t frames) override;

    void start() override;
    void stop() override;

    void SetOutputChannels(uint16_t) override;

    private:
    std::vector<unsigned char> processedBuffer;
    int epipe_count = 0;
    snd_pcm_t *alsa_dev = nullptr;

    void _init(SongFormat &format, bool realtime = false);
    template<typename T>
    int write(const T *buffer, frame_t frames);
    void drain();
    void drop();
};

#endif // ALSAOUTPUT_H
