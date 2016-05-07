#ifndef STANDARDWRAPPER_H
#define STANDARDWRAPPER_H

#include "Song.h"
#include "LoudnessFile.h"

#include <future>
#include <cfenv> // fesetround
#include <cmath> // lrint

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
    fesetround(FE_TONEAREST);\
\
    SAMPLEFORMAT* pcm = static_cast<SAMPLEFORMAT*>(bufferToFill);\
    pcm += (this->framesAlreadyRendered * this->Format.Channels) % this->count;\
\
    while(framesToRender>0 && !this->stopFillBuffer)\
    {\
        int framesToDoNow = (framesToRender/Config::FramesToRender)>0 ? Config::FramesToRender : framesToRender%Config::FramesToRender;\
\
        /* render to raw pcm*/\
        LIB_SPECIFIC_RENDER_FUNCTION;\
\
        /* audio normalization */\
        /*const*/ float absoluteGain = (numeric_limits<SAMPLEFORMAT>::max()) / (numeric_limits<SAMPLEFORMAT>::max() * this->gainCorrection);\
        /* reduce risk of clipping, remove that when using true sample peak */\
        absoluteGain -= 0.01;\
        for(unsigned int i=0; Config::useAudioNormalization && i<framesToDoNow*this->Format.Channels; i++)\
        {\
	    /* simply casting the result of the multiplication could be expensive, since the pipeline of the FPU */\
	    /* might be flushed. it not very precise either. thus better round here. */\
	    /* see: http://www.mega-nerd.com/FPcast/ */\
	    pcm[i] = static_cast<SAMPLEFORMAT>(lrint(pcm[i] * absoluteGain));\
        }\
\
        pcm += (framesToDoNow * this->Format.Channels) % this->count;\
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

    void releaseBuffer () override;
    
    frame_t getFramesRendered();

    virtual void render(pcm_t* bufferToFill, frame_t framesToRender=0) = 0;

protected:
    // used for double buffering, whenever we were unable to allocate a buffer big enough to hold the whole song in memory
    pcm_t* preRenderBuf = nullptr;

    // a flag that indicates a prematurely abort of async buffer fill
    bool stopFillBuffer = false;

    frame_t framesAlreadyRendered=0;

    template<typename WRAPPERCLASS>
    void fillBuffer(WRAPPERCLASS* context);

    // used for sound normalization
    float gainCorrection = 0.0f;

private:
    future<void> futureFillBuffer;
    
    void init() noexcept;


};

#endif // STANDARDWRAPPER_H

#include "StandardWrapper_impl.h"
