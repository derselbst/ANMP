#ifndef LIBSNDWRAPPER_H
#define LIBSNDWRAPPER_H

#include "StandardWrapper.h"

#include <sndfile.h>

/**
  * class LibSNDWrapper
  *
  * Wrapper for libsndfile, for supporting multiple common audio formats
  */
class LibSNDWrapper : public StandardWrapper<sndfile_sample_t>
{
    public:
    LibSNDWrapper(std::string filename);
    LibSNDWrapper(std::string filename, Nullable<size_t> offset, Nullable<size_t> len);

    // forbid copying
    LibSNDWrapper(LibSNDWrapper const &) = delete;
    LibSNDWrapper &operator=(LibSNDWrapper const &) = delete;

    ~LibSNDWrapper() override;

    // interface methods declaration

    void open() override;

    void close() noexcept override;

    void fillBuffer() override;

    std::vector<loop_t> getLoopArray() const noexcept override;

    frame_t getFrames() const override;

    void render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender) override;

    void buildMetadata() noexcept override;

    private:
    void init();
    SNDFILE *sndfile = nullptr;
    SF_INFO sfinfo;
    loop_t legacyLoop;

};

#endif // LIBSNDWRAPPER_H
