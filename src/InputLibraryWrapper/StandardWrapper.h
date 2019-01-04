#ifndef STANDARDWRAPPER_H
#define STANDARDWRAPPER_H

#include "Song.h"

#include <future>

/**
  * class StandardWrapper
  *
  * @brief provides common ways of opening, loading and decoding an audio file
  *
  * - automatically normalizes audio during rendering (if provided with an *.ebur128 file)
  * - decides whether to render the whole song to memory at once or not and allocates the pcm buffer (this->data) accordingly
  *
  */


/** @brief this is the common macro responsible for decoding the audio file and correctly filling the PCM buffer
  *
  * usually called within this->render() simply by putting e.g.:
  * 
  *   STANDARDWRAPPER_RENDER(float, 3rd_party_render_function(pcm, framesToDoNow))
  * 
  * "pcm" is the pointer to the buffer where the samples are being stored, as declared below
  * 
  * "framesToDoNow" is the number of frames the render function shall prepare for a given call
  * depending on the semantic of this function you may have to pass the number of items being rendered (i.e. framesToDoNow  * this->Format.Channels)
  * 
  * obviously you may not change the name of these two vars (since they are declared within the scope of the macro), however you may reoder the arguments to fit your render function (this is also the reason why we cant use template function for this purpose, at least I dont see how this should work)
  * 
  * @warning whenever changing this implementation, dont forget LibMadWrapper::render() AND FFMpegWrapper::render(), which does
  * pretty much the same thing, without using this macro
  */
#define STANDARDWRAPPER_RENDER(SAMPLEFORMAT, LIB_SPECIFIC_RENDER_FUNCTION)                                                                        \
    {                                                                                                                                             \
        auto backup = framesToRender = min(framesToRender, this->getFrames() - this->framesAlreadyRendered);                                      \
        SAMPLEFORMAT *pcm = static_cast<SAMPLEFORMAT *>(bufferToFill);                                                                            \
                                                                                                                                                  \
        while (framesToRender > 0 && !this->stopFillBuffer)                                                                                       \
        {                                                                                                                                         \
            /* render in chunks of gConfig.FramesToRender size */                                                                                 \
            int framesToDoNow = (framesToRender / gConfig.FramesToRender) > 0 ? gConfig.FramesToRender : framesToRender % gConfig.FramesToRender; \
                                                                                                                                                  \
            /* call the function whatever is responsible for decoding to raw pcm */                                                               \
            LIB_SPECIFIC_RENDER_FUNCTION;                                                                                                         \
                                                                                                                                                  \
            /* advance the pcm pointer by that many items that have just been rendered */                                                         \
            pcm += (framesToDoNow * Channels) % this->count;                                                                                      \
            this->framesAlreadyRendered += framesToDoNow;                                                                                         \
                                                                                                                                                  \
            framesToRender -= framesToDoNow;                                                                                                      \
        }                                                                                                                                         \
        framesToRender = backup;                                                                                                                  \
    }


template<typename SAMPLEFORMAT>
class StandardWrapper : public Song
{
    public:
    StandardWrapper(string filename);
    StandardWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);

    // forbid copying
    StandardWrapper(StandardWrapper const &) = delete;
    StandardWrapper &operator=(StandardWrapper const &) = delete;

    ~StandardWrapper() override;

    void releaseBuffer() noexcept override;

    frame_t getFramesRendered();

    /**
     * The render function that actually decodes and saves everything to @p bufferToFill.
     * 
     * @param bufferToFill: the buffer to be filled with PCM.
     * @param channels: number of channels, same as this->Format.Channels(), but cached for convenience
     * @param framesToRender: number of requested frames to be decoded; there may be less frames decoded if the end of file is reached, but there must not be more frames decoded
     */
    virtual void render(pcm_t *const bufferToFill, const uint32_t channels, frame_t framesToRender) = 0;

    protected:
    // used for double buffering, whenever we were unable to allocate a buffer big enough to hold the whole song in memory
    pcm_t *preRenderBuf = nullptr;

    // a flag that indicates a prematurely abort of async buffer fill
    bool stopFillBuffer = false;

    // number of frames that have been rendered to this->pcm since the song has been opened
    std::atomic<frame_t> framesAlreadyRendered = {0};

    template<typename WRAPPERCLASS>
    void fillBuffer(WRAPPERCLASS *context);

    template<typename REAL_SAMPLEFORMAT>
    void doAudioNormalization(REAL_SAMPLEFORMAT *bufferToFill, const frame_t framesToProcess);

    // used for sound normalization
    // usual range: (0,1]
    // can be slightly bigger than 1, depends on the underlying noise normalization algorithm
    // small values indicate a silent source signal resulting in bigger amplification
    // 1.0 means 0 dbFS: no amplification needed in this case ;)
    float gainCorrection = 1.0f;

    private:
    future<void> futureFillBuffer;

    void init() noexcept;
};

#endif // STANDARDWRAPPER_H

#include "StandardWrapper_impl.h"
