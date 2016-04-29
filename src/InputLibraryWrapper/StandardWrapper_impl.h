#ifndef STANDARDWRAPPER_IMPL_H
#define STANDARDWRAPPER_IMPL_H

#include "Common.h"
#include "Config.h"
#include "AtomicWrite.h"

#include <utility> // std::swap

template<typename SAMPLEFORMAT>
StandardWrapper<SAMPLEFORMAT>::StandardWrapper(string filename) : Song(filename)
{
}

template<typename SAMPLEFORMAT>
StandardWrapper<SAMPLEFORMAT>::StandardWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : Song(filename, offset, len)
{
}


template<typename SAMPLEFORMAT>
StandardWrapper<SAMPLEFORMAT>::~StandardWrapper ()
{
}


/**
 * @brief manages that Song::data holds new PCM
 * 
 * this method trys to alloc a buffer that is big enough to hold the whole PCM of whatever audiofile in memory
 * if this fails it trys to allocate a buffer big enough to hold Config::FramesToRender frames and fills this with new PCM on each call
 * 
 * if even that allocation fails, an exception will be thrown
 * 
 * @param context: holds the this pointer of that class that uses StandardWrapper; required, since we cant pass a polymorphic call to std::async
 */
template<typename SAMPLEFORMAT>
template<typename WRAPPERCLASS>
void StandardWrapper<SAMPLEFORMAT>::fillBuffer(WRAPPERCLASS* context)
{
  if(this->count == 0)
  { // no buffer allocated
    	  
      // usually this shouldnt block at all, since we only end up here after releaseBuffer() was called
      // and releaseBuffer already waits for the render thread to finish... however it doesnt hurt
      WAIT(this->futureFillBuffer);
	    
        this->count = this->getFrames() * this->Format.Channels;
	
	// try to alloc a buffer to hold the whole song's pcm in memory
        this->data = new (std::nothrow) SAMPLEFORMAT[this->count];
	
	if(this->data != nullptr)
	{ // buffer successfully allocated, fill it asynchronously

	    // (pre-)render the first few milliseconds
	    this->render(this->data, msToFrames(Config::PreRenderTime, this->Format.SampleRate));

	    // immediatly start filling the rest of the pcm buffer
	    this->futureFillBuffer = async(launch::async, &WRAPPERCLASS::render, context/*==this*/, context->data, 0/*render everything*/);

	    // allow the render thread to do his work
	    this_thread::yield();
	    
	    return;
	}
	
	// well something went wrong during alloc (not enough memory??)
	// so try to alloc at least enough to do double buffering
	// if this fails too, an exception will be thrown
	this->count = Config::FramesToRender * this->Format.Channels;
	
	this->data = new SAMPLEFORMAT[this->count];
	this->preRenderBuf = new SAMPLEFORMAT[this->count];
	
	this->render(this->data, Config::FramesToRender);
	
	goto buf_fill;
  }
  else if(this->count == Config::FramesToRender * this->Format.Channels)
  {
    WAIT(this->futureFillBuffer);
    
    // data is the consumed pcm buffer, preRenderBuf holds fresh pcm
    std::swap(this->data, this->preRenderBuf);
    
    buf_fill:
    this->futureFillBuffer = async(launch::async, &WRAPPERCLASS::render, context/*==this*/, context->preRenderBuf, Config::FramesToRender);
	
  }
  else /*i.e. this->count == this->getFrames() * this->Format.Channels*/
  {
    // Song::data already filled up with all the audiofile's PCM, nothing to do here ;)
  }
}

template<typename SAMPLEFORMAT>
void StandardWrapper<SAMPLEFORMAT>::releaseBuffer()
{
    this->stopFillBuffer=true;
    WAIT(this->futureFillBuffer);

    delete [] static_cast<SAMPLEFORMAT*>(this->data);
    this->data=nullptr;
    
    delete [] static_cast<SAMPLEFORMAT*>(this->preRenderBuf);
    this->preRenderBuf=nullptr;
    
    this->count = 0;
    this->framesAlreadyRendered=0;

    this->stopFillBuffer=false;
}
#endif