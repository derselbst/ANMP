#include "FFMpegWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"

#include <cstring>


FFMpegWrapper::FFMpegWrapper(string filename) : StandardWrapper(filename)
{
    this->Format.SampleFormat = SampleFormat_t::int16;
}

FFMpegWrapper::FFMpegWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : StandardWrapper(filename, offset, len)
{
    this->Format.SampleFormat = SampleFormat_t::int16;
}

FFMpegWrapper::~FFMpegWrapper ()
{
    this->releaseBuffer();
    this->close();
}


void FFMpegWrapper::open ()
{
    // avoid multiple calls to open()
    if(this->handle!=nullptr)
    {
        return;
    }

    // This registers all available file formats and codecs with the
    // library so they will be used automatically when a file with the
    // corresponding format/codec is opened. Note that you only need to
    // call av_register_all() once, so it's probably best to do this
    // somewhere in your startup code. If you like, it's possible to
    // register only certain individual file formats and codecs, but
    // there's usually no reason why you would have to do that.
    av_register_all();

    // The last three parameters specify the file format, buffer size and
    // format parameters. By simply specifying NULL or 0 we ask libavformat
    // to auto-detect the format and use a default buffer size.
    if(avformat_open_input(&this->handle, this->Filename.c_str(), NULL, NULL) != 0)
    {
        THROW_RUNTIME_ERROR("Failed to open file " << this->Filename);
    }

    if (avformat_find_stream_info(this->handle, NULL) < 0)
    {
        THROW_RUNTIME_ERROR("Faild to gather stream info(s) from file " << this->Filename);
    }

    // The file may contain more than on stream. Each stream can be a
    // video stream, an audio stream, a subtitle stream or something else.
    // also see 'enum CodecType' defined in avcodec.h
    // Find the first audio stream:
    this->audioStreamID = -1;

    for(unsigned int i=0; i<this->handle->nb_streams; i++)
    {
        if(this->handle->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            this->audioStreamID=i;
            break;
        }
    }
    if(this->audioStreamID==-1)
    {
        THROW_RUNTIME_ERROR("Didnt find an audio stream in file " << this->Filename);
    }

    // Get a pointer to the codec context for the audio stream
    AVCodecContext *pCodecCtx = this->handle->streams[this->audioStreamID]->codec;

    // Find the decoder for the audio stream
    AVCodec *decoder = avcodec_find_decoder(pCodecCtx->codec_id);
    if(decoder==NULL)
    {
        THROW_RUNTIME_ERROR("Codec type " <<  pCodecCtx->codec_id << " not found.")
    }

    // Open the codec found suitable for this stream in the last step
    if(avcodec_open2(pCodecCtx, decoder, NULL)<0)
    {
        THROW_RUNTIME_ERROR("Could not open decoder.");
    }

    this->Format.Channels = pCodecCtx->channels;
    this->Format.SampleRate = pCodecCtx->sample_rate;

    // get the timebase for this stream. The presentation timestamp,
    // decoding timestamp and packet duration is expressed in timestamp
    // as unit:
    // e.g. if timebase is 1/90000, a packet with duration 4500
    // is 4500 * 1/90000 seconds long, that is 0.05 seconds == 20 ms.
    AVRational& tb = this->handle->streams[this->audioStreamID]->time_base;
    double time_base =  (double)tb.num / (double)tb.den;
    double msDuration = (double)this->handle->streams[this->audioStreamID]->duration * time_base * 1000.0;
    this->fileLen = msDuration;


    if(pCodecCtx->channel_layout==0)
    {
        pCodecCtx->channel_layout = av_get_default_channel_layout( pCodecCtx->channels );
    }

    AVSampleFormat& sfmt=pCodecCtx->sample_fmt;

    // Set up SWR context once you've got codec information
    this->swr = swr_alloc();
    av_opt_set_int(this->swr, "in_channel_layout",  pCodecCtx->channel_layout, 0);
    av_opt_set_int(this->swr, "out_channel_layout", pCodecCtx->channel_layout,  0);
    av_opt_set_int(this->swr, "in_sample_rate",     pCodecCtx->sample_rate, 0);
    av_opt_set_int(this->swr, "out_sample_rate",    pCodecCtx->sample_rate, 0);
    av_opt_set_sample_fmt(this->swr, "in_sample_fmt",  sfmt, 0);
    av_opt_set_sample_fmt(this->swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
    swr_init(this->swr);

}

void FFMpegWrapper::close() noexcept
{
    if(this->swr!=nullptr)
    {
        swr_free(&this->swr);
        this->swr = nullptr;
    }

    if(this->handle!=nullptr)
    {
        if(this->audioStreamID != -1)
        {
            avcodec_close(this->handle->streams[this->audioStreamID]->codec);
        }
        avformat_close_input(&this->handle);
        this->handle=nullptr;
    }
}

void FFMpegWrapper::fillBuffer()
{
    StandardWrapper::fillBuffer(this);
}

// TODO: this crap is not guaranteed to generate exactly framesToRender frames, there might by more thrown out, which is bad in case of a limited buffer
void FFMpegWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    int framesToDo;
    if(framesToRender==0)
    {
        /* render rest of file */
        framesToDo = this->getFrames()-this->framesAlreadyRendered;
    }
    else
    {
        framesToDo = min(framesToRender, this->getFrames()-this->framesAlreadyRendered);
    }

    //data packet read from the stream
    AVPacket packet;
    av_init_packet(&packet);

//     const int buffer_size=192000 + FF_INPUT_BUFFER_PADDING_SIZE; // AVCODEC_MAX_AUDIO_FRAME_SIZE
//     uint8_t buffer[buffer_size];
//     packet.data=buffer;
//     packet.size =buffer_size;

    //frame, where the decoded data will be written
    AVFrame *frame=av_frame_alloc();
    if(frame==NULL)
    {
        THROW_RUNTIME_ERROR("Cannot allocate AVFrame.")
    }

    int frameFinished=0;

    // cast it to stupid byte pointer
    int16_t* pcm = static_cast<int16_t*>(bufferToFill);
    pcm += this->framesAlreadyRendered * this->Format.Channels;

    while(!this->stopFillBuffer &&
            framesToDo > 0 &&
            pcm < static_cast<int16_t*>(bufferToFill) + this->count &&
            av_read_frame(this->handle,&packet)==0
         )
    {
        if(packet.stream_index==this->audioStreamID)
        {
            int ret = avcodec_decode_audio4(this->handle->streams[this->audioStreamID]->codec, frame, &frameFinished, &packet);

            if (ret < 0)
            {
                THROW_RUNTIME_ERROR("Error decoding audio frame"/* (" << av_err2str(ret) << ")"*/);
            }

            if(frameFinished != 0)
            {
                // make sure we dont run over buffer
                int framesToConvert = min(frame->nb_samples, framesToDo);
                
                swr_convert(this->swr, reinterpret_cast<uint8_t**>(&pcm), framesToConvert, const_cast<const uint8_t**>(frame->extended_data), framesToConvert);

                size_t unpadded_linesize = framesToConvert * frame->channels;// * sizeof(int16_t); //* av_get_bytes_per_sample((AVSampleFormat)frame->format);
                pcm += unpadded_linesize;

                framesToDo -= framesToConvert;
                
                if(framesToDo < 0)
                {
                    CLOG(LogLevel::ERROR, "THIS SHOULD NEVER HAPPEN! bufferoverrun ffmpegwrapper")
                }
                
                this->framesAlreadyRendered += framesToConvert;
            }
            else
            {
                // no clue what to do here...
            }
        }
        else
        {
            //someother stream,probably a video stream
        }

        av_packet_unref(&packet);
    }
    av_frame_free(&frame);
}

frame_t FFMpegWrapper::getFrames () const
{
    size_t totalFrames = this->fileLen.hasValue ? msToFrames(this->fileLen.Value, this->Format.SampleRate) : 0;

    return totalFrames;
}

void FFMpegWrapper::buildMetadata() noexcept
{

}
