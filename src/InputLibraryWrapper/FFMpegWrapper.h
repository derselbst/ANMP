#ifndef FFMPEGWRAPPER_H
#define FFMPEGWRAPPER_H

#include "StandardWrapper.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

/**
  * class FFMpegWrapper
  *
  */

class FFMpegWrapper : public StandardWrapper<int16_t>
{
public:

    // Constructors/Destructors
    //


    /**
     * Empty Constructor
     */
    FFMpegWrapper(string filename);
    FFMpegWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

    /**
     * Empty Destructor
     */
    virtual ~FFMpegWrapper ();

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
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    frame_t getFrames () const override;

    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;

    void buildMetadata() override;
private:
    AVFormatContext *handle = nullptr;
    SwrContext *swr = nullptr;
    
    int audioStreamID = -1;
};

#endif // FFMPEGWRAPPER_H