#ifndef STANDARDWRAPPER_H
#define STANDARDWRAPPER_H

#include "Song.h"

extern "C"
{
  #include <vgmstream.h>
}
#include <future>
// struct VGMSTREAM;

/**
  * class StandardWrapper
  *
  */

#define STANDARDWRAPPER_RENDER(SAMPLEFORMAT, LIB_SPECIFIC_RENDER_FUNCTION)\
    if(framesToRender==0)\
    {\
        /* render rest of file */ \
        framesToRender = this->getFrames()-this->framesAlreadyRendered;\
    }\
    else\
    {\
        framesToRender = min(framesToRender, this->getFrames()-this->framesAlreadyRendered);\
    }\
\
    SAMPLEFORMAT* pcm = static_cast<SAMPLEFORMAT*>(this->data);\
    pcm += this->framesAlreadyRendered * this->Format.Channels;\
\
    int framesToDoNow;\
    while(framesToRender>0 && !this->stopFillBuffer)\
    {\
        framesToDoNow = (framesToRender/Config::FramesToRender)>0 ? Config::FramesToRender : framesToRender%Config::FramesToRender;\
\
        LIB_SPECIFIC_RENDER_FUNCTION;\
\
        pcm += framesToDoNow * this->Format.Channels;\
        this->framesAlreadyRendered += framesToDoNow;\
\
        framesToRender -= framesToDoNow;\
    }\


template<typename SAMPLEFORMAT>
class StandardWrapper : public Song
{
public:

    StandardWrapper(string filename);
    StandardWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    
    virtual ~StandardWrapper ();


    /**
     */
    void releaseBuffer () override;
    
    virtual void render(frame_t framesToRender=0) = 0;

protected:
  // a flag that indicates a prematurely abort of async buffer fill
  bool stopFillBuffer = false;

  frame_t framesAlreadyRendered=0;

  template<typename WRAPPERCLASS>
  void fillBuffer(WRAPPERCLASS* context);


private:
  future<void> futureFillBuffer;


};

#endif // STANDARDWRAPPER_H

#include "StandardWrapper_impl.h"