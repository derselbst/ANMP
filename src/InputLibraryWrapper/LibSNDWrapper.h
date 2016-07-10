#ifndef LIBSNDWRAPPER_H
#define LIBSNDWRAPPER_H

#include "StandardWrapper.h"

#include <sndfile.h>
#include <future>


/**
  * class LibSNDWrapper
  *
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

    /**
     * opens the current file using the corresponding lib
     */
    void open () override;


    /**
     */
    void close () noexcept override;


    /** PCM buffer fill call to underlying library goes here
     */
    void fillBuffer () override;

    /**
     * @return vector
     */
    vector<loop_t> getLoopArray () const noexcept override;


    /**
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    frame_t getFrames () const override;

    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;

    void buildMetadata() noexcept override;
private:
    SNDFILE* sndfile = nullptr;
    SF_INFO sfinfo;
};

#endif // LIBSNDWRAPPER_H
