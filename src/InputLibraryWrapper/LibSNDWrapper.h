#ifndef LIBSNDWRAPPER_H
#define LIBSNDWRAPPER_H

#include "StandardWrapper.h"

#include <sndfile.h>


/**
  * class LibSNDWrapper
  *
  * Wrapper for libsndfile, for supporting multiple common audio formats
  */
class LibSNDWrapper : public StandardWrapper<int32_t>
{
public:
    LibSNDWrapper(string filename);
    LibSNDWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

    // forbid copying
    LibSNDWrapper(LibSNDWrapper const&) = delete;
    LibSNDWrapper& operator=(LibSNDWrapper const&) = delete;

    virtual ~LibSNDWrapper ();

    // interface methods declaration

    void open () override;

    void close () noexcept override;

    void fillBuffer () override;

    vector<loop_t> getLoopArray () const noexcept override;

    frame_t getFrames () const override;

    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;

    void buildMetadata() noexcept override;
private:
    SNDFILE* sndfile = nullptr;
    SF_INFO sfinfo;
};

#endif // LIBSNDWRAPPER_H
