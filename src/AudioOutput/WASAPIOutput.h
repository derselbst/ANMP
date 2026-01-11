#pragma once

#ifdef _WIN32

#include "IAudioOutput.h"

#include <memory>

class WASAPIOutput : public IAudioOutput
{
    public:
    WASAPIOutput();
    WASAPIOutput(WASAPIOutput const &) = delete;
    WASAPIOutput &operator=(WASAPIOutput const &) = delete;
    ~WASAPIOutput() override;

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
    template<typename T>
    int writeInternal(const T *buffer, frame_t frames);

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // _WIN32
