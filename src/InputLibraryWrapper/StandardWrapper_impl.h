#ifndef STANDARDWRAPPER_IMPL_H
#define STANDARDWRAPPER_IMPL_H

#include "Common.h"
#include "Config.h"
#include "AtomicWrite.h"

#include <utility> // std::swap

template<typename SAMPLEFORMAT>
StandardWrapper<SAMPLEFORMAT>::StandardWrapper(string filename) : Song(filename)
{
    this->init();
}

template<typename SAMPLEFORMAT>
StandardWrapper<SAMPLEFORMAT>::StandardWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : Song(filename, offset, len)
{
    this->init();
}

template<typename SAMPLEFORMAT>
void StandardWrapper<SAMPLEFORMAT>::init() noexcept
{
    this->gainCorrection = LoudnessFile::read(this->Filename);
}

template<typename SAMPLEFORMAT>
StandardWrapper<SAMPLEFORMAT>::~StandardWrapper ()
{
}


/**
 * @brief manages that Song::data holds new PCM
 *
 * this method trys to alloc a buffer that is big enough to hold the whole PCM of whatever audiofile in memory
 * if this fails it trys to allocate a buffer big enough to hold gConfig.FramesToRender frames and fills this with new PCM on each call
 *
 * if even that allocation fails, an exception will be thrown
 *
 * @param context holds the this pointer of that class that uses StandardWrapper; required, since we cant pass a polymorphic call to std::async
 */
template<typename SAMPLEFORMAT>
template<typename WRAPPERCLASS>
void StandardWrapper<SAMPLEFORMAT>::fillBuffer(WRAPPERCLASS* context)
{
    if(this->count == static_cast<size_t>(this->getFrames()) * this->Format.Channels())
    {
        // Song::data already filled up with all the audiofile's PCM, nothing to do here (most likely case)
        return;
    }
    else if(this->count == 0) // no buffer allocated?
    {
        if(!this->Format.IsValid())
        {
            THROW_RUNTIME_ERROR("Failed to allocate buffer: SongFormat not valid!");
        }

        // usually this shouldnt block at all, since we only end up here after releaseBuffer() was called
        // and releaseBuffer already waits for the render thread to finish... however it doesnt hurt
        WAIT(this->futureFillBuffer);
        
        size_t itemsToAlloc = 0;
        if(gConfig.RenderWholeSong)
        {
            itemsToAlloc = this->getFrames() * this->Format.Channels();

            // try to alloc a buffer to hold the whole song's pcm in memory
            this->data = new (std::nothrow) SAMPLEFORMAT[itemsToAlloc];
            if(this->data != nullptr) // buffer successfully allocated, fill it asynchronously
            {
                this->count = itemsToAlloc;

                // (pre-)render the first few milliseconds
                this->render(this->data, msToFrames(gConfig.PreRenderTime, this->Format.SampleRate));

                // immediatly start filling the rest of the pcm buffer
                this->futureFillBuffer = async(launch::async, &WRAPPERCLASS::render, context/*==this*/, context->data, 0/*render everything*/);

                // allow the render thread to do his work
                this_thread::yield();

                return;
            }
        }

        // well either we shall not render whole song once or something went wrong during alloc (not enough memory??)
        // so try to alloc at least enough to do double buffering
        // if this fails too, an exception will be thrown
        itemsToAlloc = gConfig.FramesToRender * this->Format.Channels();

        try
        {
            this->data = new SAMPLEFORMAT[itemsToAlloc];
            this->preRenderBuf = new SAMPLEFORMAT[itemsToAlloc];
            this->count = itemsToAlloc;
        }
        catch(const std::bad_alloc& e)
        {
            this->releaseBuffer();
            throw;
        }
        
        this->render(this->data, gConfig.FramesToRender);
    }
    else // only small buffer allocated, i.e. this->count == gConfig.FramesToRender * this->Format.Channels
    {
        WAIT(this->futureFillBuffer);

        // data is the consumed pcm buffer, preRenderBuf holds fresh pcm
        std::swap(this->data, this->preRenderBuf);
    }

    this->futureFillBuffer = async(launch::async, &WRAPPERCLASS::render, context/*==this*/, context->preRenderBuf, gConfig.FramesToRender);
}

template<typename SAMPLEFORMAT>
void StandardWrapper<SAMPLEFORMAT>::releaseBuffer() noexcept
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

template<typename SAMPLEFORMAT>
frame_t StandardWrapper<SAMPLEFORMAT>::getFramesRendered()
{
    return this->framesAlreadyRendered;
}
#endif
