#ifndef STANDARDWRAPPER_IMPL_H
#define STANDARDWRAPPER_IMPL_H

#include "Common.h"
#include "Config.h"
#include "AtomicWrite.h"


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

template<typename SAMPLEFORMAT>
template<typename WRAPPERCLASS>
void StandardWrapper<SAMPLEFORMAT>::fillBuffer(WRAPPERCLASS* context)
{
    if(this->data==nullptr)
    {
        this->count = this->getFrames() * this->Format.Channels;
        this->data = new SAMPLEFORMAT[this->count];

        // usually this shouldnt block at all
        WAIT(this->futureFillBuffer);

        // (pre-)render the first few milliseconds
        this->render(msToFrames(Config::PreRenderTime, this->Format.SampleRate));

        // immediatly start filling the pcm buffer
        this->futureFillBuffer = async(launch::async, &WRAPPERCLASS::render, context, 0);

        // allow the render thread to do his work
        this_thread::yield();
    }
}

template<typename SAMPLEFORMAT>
void StandardWrapper<SAMPLEFORMAT>::releaseBuffer()
{
    this->stopFillBuffer=true;
    WAIT(this->futureFillBuffer);

    delete [] static_cast<SAMPLEFORMAT*>(this->data);
    this->data=nullptr;
    this->count = 0;
    this->framesAlreadyRendered=0;

    this->stopFillBuffer=false;
}
#endif