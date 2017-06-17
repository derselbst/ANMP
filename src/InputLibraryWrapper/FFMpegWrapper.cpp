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
    // format parameters. By simply specifying nullptr or 0 we ask libavformat
    // to auto-detect the format and use a default buffer size.
    if(avformat_open_input(&this->handle, this->Filename.c_str(), nullptr, nullptr) != 0)
    {
        THROW_RUNTIME_ERROR("Failed to open file " << this->Filename);
    }

    if (avformat_find_stream_info(this->handle, nullptr) < 0)
    {
        THROW_RUNTIME_ERROR("Faild to gather stream info(s) from file " << this->Filename);
    }

    // The file may contain more than on stream. Each stream can be a
    // video stream, an audio stream, a subtitle stream or something else.
    // also see 'enum CodecType' defined in avcodec.h
    // Find the first audio stream:
    this->audioStreamID = -1;

    int ret = av_find_best_stream(this->handle, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (ret < 0)
    {
        THROW_RUNTIME_ERROR("Could not find " << av_get_media_type_string(AVMEDIA_TYPE_AUDIO) << " stream in \"" << this->Filename << "\"");
    }
    else
    {
        this->audioStreamID = ret;
    }

    AVStream* audioStream = this->handle->streams[this->audioStreamID];
    
    // Get a pointer to the codec context for the audio stream
    AVCodecContext *pCodecCtx = audioStream->codec;

    // Find the decoder for the audio stream
    AVCodec *decoder = avcodec_find_decoder(pCodecCtx->codec_id);
    if(decoder==nullptr)
    {
        THROW_RUNTIME_ERROR("Codec type " <<  pCodecCtx->codec_id << " not found.")
    }

    // Open the codec found suitable for this stream in the last step
    if(avcodec_open2(pCodecCtx, decoder, nullptr)<0)
    {
        THROW_RUNTIME_ERROR("Could not open decoder.");
    }

    this->Format.SetVoices(1);
    this->Format.VoiceChannels[0] = pCodecCtx->channels;
    this->Format.SampleRate = pCodecCtx->sample_rate;

    // we now have to retrieve the playduration of this file
    // if it's possible to retrieve this from the currently decoded audio stream itself, we prefer that way
    if(audioStream->duration != AV_NOPTS_VALUE && audioStream->duration >= 0)
    {
        // get the timebase for this stream. The presentation timestamp,
        // decoding timestamp and packet duration is expressed in timestamp
        // as unit:
        // e.g. if timebase is 1/90000, a packet with duration 4500
        // is 4500 * 1/90000 seconds long, that is 0.05 seconds == 50 ms.
        AVRational& tb = audioStream->time_base;
        double time_base =  (double)tb.num / (double)tb.den;
        double msDuration = audioStream->duration * time_base * 1000.0;
        this->fileLen = msDuration;
    }
    else if(this->handle->duration != AV_NOPTS_VALUE && this->handle->duration >= 0)
    {
        // this line seems to be completely pointless
        // stolen from FFMPEG/libavformat/dump.c:558
        int64_t duration = this->handle->duration + (this->handle->duration <= INT64_MAX - 5000 ? 5000 : 0);
        size_t msDuration = (static_cast<double>(duration) / AV_TIME_BASE)/*sec*/ * 1000;
        this->fileLen = msDuration;
    }
    else
    {
        THROW_RUNTIME_ERROR("FFMpeg doesnt specify duration for this file."); // either this, or there is some other weird way of getting the duration, which is not implemented
    }

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

int FFMpegWrapper::decode_packet(int16_t* (&pcm), int& framesToDo, int& got_frame, AVPacket& pkt, AVFrame* (&frame))
{
    int decoded = pkt.size;

    got_frame = 0;

    if (pkt.stream_index == this->audioStreamID)
    {
        /* decode audio frame */
        int ret = avcodec_decode_audio4(this->handle->streams[this->audioStreamID]->codec, frame, &got_frame, &pkt);
        if (ret < 0)
        {
            CLOG(LogLevel_t::Error, "Failed decoding audio frame");
        }

        /* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Sample: fate-suite/lossless-audio/luckynight-partial.shn
         * Also, some decoders might over-read the packet. */
        decoded = min(ret, pkt.size);

        if(got_frame != 0)
        {
            // make sure we dont run over buffer
            int framesToConvert = min(frame->nb_samples, framesToDo);

            swr_convert(this->swr, reinterpret_cast<uint8_t**>(&pcm), framesToConvert, const_cast<const uint8_t**>(frame->extended_data), framesToConvert);

            size_t unpadded_linesize = framesToConvert * frame->channels;// * sizeof(int16_t); //* av_get_bytes_per_sample((AVSampleFormat)frame->format);
            pcm += unpadded_linesize;

            framesToDo -= framesToConvert;

            if(framesToDo < 0)
            {
                CLOG(LogLevel_t::Error, "THIS SHOULD NEVER HAPPEN! bufferoverrun ffmpegwrapper")
            }

            this->framesAlreadyRendered += framesToConvert;
        }
    }
    else
    {
        // dont care
    }

    /* If we use frame reference counting, we own the data and need
     * to de-reference it when we don't use it anymore */
//     if (*got_frame && refcount)
//         av_frame_unref(frame);

    return decoded;
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

    /* initialize packet, set data to nullptr, let the demuxer fill it */
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;


    //frame, where the decoded data will be written
    AVFrame *frame=av_frame_alloc();
    if(frame==nullptr)
    {
        THROW_RUNTIME_ERROR("Cannot allocate AVFrame.")
    }

    int frameFinished=0;

    // int16 because we told swr to convert everything to int16
    int16_t* pcm = static_cast<int16_t*>(bufferToFill);
    bufferToFill = (pcm += this->framesAlreadyRendered * this->Format.Channels());


    while(!this->stopFillBuffer &&
            framesToDo > 0 &&
            pcm < static_cast<int16_t*>(bufferToFill) + this->count
         )
    {

        /* read frames from the file */
        int ret = av_read_frame(this->handle,&packet);
        if (ret < 0)
        {
            if (ret == AVERROR_EOF || avio_feof(this->handle->pb))
            {
                CLOG(LogLevel_t::Debug, "AVERROR_EOF");
                break;
            }
            if (this->handle->pb!=nullptr && this->handle->pb->error)
            {
                CLOG(LogLevel_t::Debug, "pb error: " << this->handle->pb->error);
                break;
            }
        }

        AVPacket orig_pkt = packet;
        do
        {
            ret = decode_packet(pcm, framesToDo, frameFinished, packet, frame);
            if (ret < 0)
                break;
            packet.data += ret;
            packet.size -= ret;
        } while (packet.size > 0);

        av_packet_unref(&orig_pkt);
    }

    /* flush any cached frames */
    packet.data = nullptr;
    packet.size = 0;
    do
    {
        decode_packet(pcm, framesToDo, frameFinished, packet, frame);
    } while (frameFinished);


    av_frame_free(&frame);
    
    
    this->doAudioNormalization(static_cast<int16_t*>(bufferToFill), framesToRender);
}

frame_t FFMpegWrapper::getFrames () const
{
    frame_t totalFrames = this->fileLen.hasValue ? msToFrames(this->fileLen.Value, this->Format.SampleRate) : 0;

    return totalFrames;
}

void FFMpegWrapper::buildMetadata() noexcept
{
    AVDictionaryEntry* tag = nullptr;
    
    tag = av_dict_get(this->handle->metadata, "artist", nullptr, AV_DICT_IGNORE_SUFFIX);
    if(tag != nullptr)
    {
        this->Metadata.Artist = tag->value;
    }
    
    tag = av_dict_get(this->handle->metadata, "title", nullptr, AV_DICT_IGNORE_SUFFIX);
    if(tag != nullptr)
    {
        this->Metadata.Title = tag->value;
    }
    
    tag = av_dict_get(this->handle->metadata, "album", nullptr, AV_DICT_IGNORE_SUFFIX);
    if(tag != nullptr)
    {
        this->Metadata.Album = tag->value;
    }
    
    tag = av_dict_get(this->handle->metadata, "composer", nullptr, AV_DICT_IGNORE_SUFFIX);
    if(tag != nullptr)
    {
        this->Metadata.Composer = tag->value;
    }
    
    tag = av_dict_get(this->handle->metadata, "track", nullptr, AV_DICT_IGNORE_SUFFIX);
    if(tag != nullptr)
    {
        this->Metadata.Track = tag->value;
    }
    
    tag = av_dict_get(this->handle->metadata, "date", nullptr, AV_DICT_IGNORE_SUFFIX);
    if(tag != nullptr)
    {
        this->Metadata.Year = tag->value;
    }
    
    tag = av_dict_get(this->handle->metadata, "genre", nullptr, AV_DICT_IGNORE_SUFFIX);
    if(tag != nullptr)
    {
        this->Metadata.Genre = tag->value;
    }
    
    tag = av_dict_get(this->handle->metadata, "comment", nullptr, AV_DICT_IGNORE_SUFFIX);
    if(tag != nullptr)
    {
        this->Metadata.Comment = tag->value;
    }
}
