#pragma once

#include "StandardWrapper.h"



namespace openmpt
{
    class module;
}

/**
  * class OpenMPTWrapper
  *
  */
class OpenMPTWrapper : public StandardWrapper<float>
{
    public:
    OpenMPTWrapper(string filename);
    OpenMPTWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len);
    void initAttr();

    // forbid copying
    OpenMPTWrapper(OpenMPTWrapper const &) = delete;
    OpenMPTWrapper &operator=(OpenMPTWrapper const &) = delete;

    ~OpenMPTWrapper() override;

    // interface methods declaration

    void open() override;

    void close() noexcept override;

    frame_t getFrames() const override;

    void render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender) override;

    void buildMetadata() noexcept override;
    private:
    openmpt::module *handle = nullptr;
};
