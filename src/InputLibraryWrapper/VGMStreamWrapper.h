#ifndef VGMSTREAMWRAPPER_H
#define VGMSTREAMWRAPPER_H

#include "Song.h"

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

class VGMStreamWrapper : public Song
{
public:

    VGMStreamWrapper (string filename, size_t fileOffset=0, size_t fileLen=0);

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

private:
  VGMSTREAM * handle=nullptr;
  
  future<void> futureFillBuffer;
  // a flag that indicates a prematurely abort of async buffer fill
  bool stopFillBuffer = false;

  void asyncFillBuffer();

};

#endif // VGMSTREAMWRAPPER_H
