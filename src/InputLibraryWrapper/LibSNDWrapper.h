#ifndef LIBSNDWRAPPER_H
#define LIBSNDWRAPPER_H

#include "StandardWrapper.h"

#include <sndfile.h>

// depending whether the audio file we contains floats, doubles or ints, access the decoded pcm as int or float
union sndfile_sample_t
{
    // classes from the outside will either see an int32 or a float here. they dont see this union. they cant acces the members via f and i members. there require float to be 32 bits, to avoid having any padding bits that would be treated as audio.
    static_assert(sizeof(float) == sizeof(int32_t), "sizeof(float) != sizeof(int32_t), sry, this code wont work");

    float f;
    int32_t i;
};

/**
  * class LibSNDWrapper
  *
  * Wrapper for libsndfile, for supporting multiple common audio formats
  */
class LibSNDWrapper : public StandardWrapper<sndfile_sample_t>
{
    public:
    LibSNDWrapper(string filename);
    LibSNDWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len);

    // forbid copying
    LibSNDWrapper(LibSNDWrapper const &) = delete;
    LibSNDWrapper &operator=(LibSNDWrapper const &) = delete;

    ~LibSNDWrapper() override;

    // interface methods declaration

    void open() override;

    void close() noexcept override;

    void fillBuffer() override;

    vector<loop_t> getLoopArray() const noexcept override;

    frame_t getFrames() const override;

    void render(pcm_t *bufferToFill, frame_t framesToRender = 0) override;

    void buildMetadata() noexcept override;

    private:
    SNDFILE *sndfile = nullptr;
    SF_INFO sfinfo;
};

#endif // LIBSNDWRAPPER_H
