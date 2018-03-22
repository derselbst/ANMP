#ifndef FFMPEGWRAPPER_H
#define FFMPEGWRAPPER_H

#include "StandardWrapper.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
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

    // forbid copying
    FFMpegWrapper(FFMpegWrapper const &) = delete;
    FFMpegWrapper &operator=(FFMpegWrapper const &) = delete;

    virtual ~FFMpegWrapper();

    // interface methods declaration

    void open() override;

    void close() noexcept override;

    void fillBuffer() override;

    frame_t getFrames() const override;

    void render(pcm_t *bufferToFill, frame_t framesToRender = 0) override;

    void buildMetadata() noexcept override;

    private:
    AVFormatContext *handle = nullptr;
    SwrContext *swr = nullptr;

    int audioStreamID = -1;

    int decode_packet(int16_t *(&pcm), int &framesToDo, int &got_frame, AVPacket &pkt, AVFrame *(&frame));
};

#endif // FFMPEGWRAPPER_H
