#ifndef PortAudioOutput_H
#define PortAudioOutput_H

#include "IAudioOutput.h"

#include <memory>

/**
  * class PortAudioOutput
  *
  * A wrapper library for portaudio, enabling ANMP for crossplatform playback
  */
class PortAudioOutput : public IAudioOutput
{
    public:
    PortAudioOutput();

    // forbid copying
    PortAudioOutput(PortAudioOutput const &) = delete;
    PortAudioOutput &operator=(PortAudioOutput const &) = delete;

    virtual ~PortAudioOutput();


    // interface methods declaration

    void open() override;

    void init(SongFormat &format, bool realtime = false) override;

    void close() override;

    int write(const float *buffer, frame_t frames) override;

    int write(const int16_t *buffer, frame_t frames) override;

    int write(const int32_t *buffer, frame_t frames) override;

    void start() override;
    void stop() override;

    void SetOutputChannels(uint8_t) override;

    private:
    struct Impl;
    std::unique_ptr<Impl> d;

    template<typename T>
    int write(const T *buffer, frame_t frames);
    void _init(SongFormat &format, bool realtime = false);
    void drain();
    void drop();
};

#endif // PortAudioOutput_H
