#ifndef VGMSTREAMWRAPPER_H
#define VGMSTREAMWRAPPER_H

#include "StandardWrapper.h"

extern "C" {
#include <vgmstream.h>
}

/**
  * class VGMStreamWrapper
  *
  * Wrapper for VGMStream, supporting multiple streamed audio formats used in video games
  */

class VGMStreamWrapper : public StandardWrapper<int16_t>
{
    public:
    VGMStreamWrapper(string filename);
    VGMStreamWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len);
    void initAttr();

    // forbid copying
    VGMStreamWrapper(VGMStreamWrapper const &) = delete;
    VGMStreamWrapper &operator=(VGMStreamWrapper const &) = delete;

    virtual ~VGMStreamWrapper();


    // interface methods declaration

    void open() override;

    void close() noexcept override;

    void fillBuffer() override;

    vector<loop_t> getLoopArray() const noexcept override;

    frame_t getFrames() const override;

    void render(pcm_t *bufferToFill, frame_t framesToRender = 0) override;

    void buildMetadata() noexcept override;

    private:
    VGMSTREAM *handle = nullptr;
};

#endif // VGMSTREAMWRAPPER_H
