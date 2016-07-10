#ifndef VGMSTREAMWRAPPER_H
#define VGMSTREAMWRAPPER_H

#include "StandardWrapper.h"

extern "C"
{
#include <vgmstream.h>
}
#include <future>
// struct VGMSTREAM;

/**
  * class VGMStreamWrapper
  *
  */

class VGMStreamWrapper : public StandardWrapper<int16_t>
{
public:

    VGMStreamWrapper(string filename);
    VGMStreamWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

    // forbid copying
    VGMStreamWrapper(VGMStreamWrapper const&) = delete;
    VGMStreamWrapper& operator=(VGMStreamWrapper const&) = delete;

    virtual ~VGMStreamWrapper ();


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
    VGMSTREAM * handle=nullptr;


};

#endif // VGMSTREAMWRAPPER_H
