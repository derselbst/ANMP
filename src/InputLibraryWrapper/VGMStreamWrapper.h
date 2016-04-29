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
    
    virtual ~VGMStreamWrapper ();


    // interface methods declaration

    /**
     * opens the current file using the corresponding lib
     */
    void open () override;


    /**
     */
    void close () override;


    /** PCM buffer fill call to underlying library goes here
     */
    void fillBuffer () override;


    /**
     */
    void releaseBuffer () override;


    /**
     * @return vector
     */
    vector<loop_t> getLoopArray () const override;


    /**
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    frame_t getFrames () const override;

  void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;
  
  void buildMetadata() override;
    
private:
  VGMSTREAM * handle=nullptr;
  

};

#endif // VGMSTREAMWRAPPER_H
