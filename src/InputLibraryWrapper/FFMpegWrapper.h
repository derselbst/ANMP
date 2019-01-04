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
    FFMpegWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len);
    void initAttr();

    // forbid copying
    FFMpegWrapper(FFMpegWrapper const &) = delete;
    FFMpegWrapper &operator=(FFMpegWrapper const &) = delete;

    ~FFMpegWrapper() override;

    // interface methods declaration

    void open() override;

    void close() noexcept override;

    void fillBuffer() override;

    frame_t getFrames() const override;

    void render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender) override;

    void buildMetadata() noexcept override;

    private:
    AVFormatContext *handle = nullptr;
    SwrContext *swr = nullptr;
    AVCodecContext *codecCtx = nullptr;

    // things needed to save the current decoding state, to allow this->render() be called multiple times without causing interrupts in the decoded audio

    AVFrame *frame = nullptr;
    //data packet read from the stream
    AVPacket *packet = nullptr;
    std::vector<int16_t> tmpSwrBuf;

    int audioStreamID = -1;

    int decode_packet(int16_t *(&pcm), int &framesToDo);
};

#endif // FFMPEGWRAPPER_H
