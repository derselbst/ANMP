#include "FFMpegWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Config.h"

#include <cstring>
#include <utility>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

FFMpegWrapper::FFMpegWrapper(std::string filename)
: StandardWrapper(std::move(filename))
{
    this->Format.SampleFormat = SampleFormat_t::int16;
}

FFMpegWrapper::FFMpegWrapper(std::string filename, Nullable<size_t> offset, Nullable<size_t> len)
: StandardWrapper(std::move(filename), offset, len)
{
    this->Format.SampleFormat = SampleFormat_t::int16;
}

FFMpegWrapper::~FFMpegWrapper()
{
    this->releaseBuffer();
    this->close();
}


void FFMpegWrapper::open()
{
    // avoid multiple calls to open()
    if (this->handle != nullptr)
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
    if (avformat_open_input(&this->handle, this->Filename.c_str(), nullptr, nullptr) != 0)
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

    AVStream *audioStream = this->handle->streams[this->audioStreamID];

    // Get a pointer to the codec context for the audio stream
    AVCodecParameters *pCodecPar = audioStream->codecpar;

    // Find the decoder for the audio stream
    AVCodec *decoder = avcodec_find_decoder(pCodecPar->codec_id);
    if (decoder == nullptr)
    {
        THROW_RUNTIME_ERROR("Codec type " << pCodecPar->codec_id << " not found.")
    }

    if ((this->codecCtx = avcodec_alloc_context3(decoder)) == nullptr)
    {
        THROW_RUNTIME_ERROR("Could not allocate audio codec context");
    }

    /* Initialize the stream parameters with demuxer information.
     * This is needed for aac decoding for some reason.
     */
    if (avcodec_parameters_to_context(this->codecCtx, audioStream->codecpar) < 0)
    {
        THROW_RUNTIME_ERROR("avcodec_parameters_to_context() failed");
    }

    // This is not needed anymore above FFMpeg version 4.0
#if LIBAVCODEC_VERSION_INT < 3805796
    // Se timebase correct
    av_codec_set_pkt_timebase(this->codecCtx, audioStream->time_base);
#endif

    // Open the codec found suitable for this stream in the last step
    if (avcodec_open2(this->codecCtx, decoder, nullptr) < 0)
    {
        THROW_RUNTIME_ERROR("Could not open decoder.");
    }

    this->Format.SetVoices(1);
    this->Format.VoiceChannels[0] = pCodecPar->channels;
    this->Format.SampleRate = pCodecPar->sample_rate;

    // we now have to retrieve the playduration of this file
    // if it's possible to retrieve this from the currently decoded audio stream itself, we prefer that way
    if (audioStream->duration != AV_NOPTS_VALUE && audioStream->duration >= 0)
    {
        // get the timebase for this stream. The presentation timestamp,
        // decoding timestamp and packet duration is expressed in timestamp
        // as unit:
        // e.g. if timebase is 1/90000, a packet with duration 4500
        // is 4500 * 1/90000 seconds long, that is 0.05 seconds == 50 ms.
        AVRational &tb = audioStream->time_base;
        double time_base = (double)tb.num / (double)tb.den;
        double msDuration = audioStream->duration * time_base * 1000.0;
        this->fileLen = msDuration;
    }
    else if (this->handle->duration != AV_NOPTS_VALUE && this->handle->duration >= 0)
    {
        // this line seems to be completely pointless
        // stolen from FFMPEG/libavformat/dump.c:558
        int64_t duration = this->handle->duration + (this->handle->duration <= INT64_MAX - 5000 ? 5000 : 0);
        size_t msDuration = (static_cast<double>(duration) / AV_TIME_BASE) /*sec*/ * 1000;
        this->fileLen = msDuration;
    }
    else
    {
        THROW_RUNTIME_ERROR("FFMpeg doesnt specify duration for this file."); // either this, or there is some other weird way of getting the duration, which is not implemented
    }

    if (pCodecPar->channel_layout == 0)
    {
        pCodecPar->channel_layout = av_get_default_channel_layout(pCodecPar->channels);
    }

    // Set up SWR context once you've got codec information
    if ((this->swr = swr_alloc()) == nullptr)
    {
        THROW_RUNTIME_ERROR("Cannot alloc swr.");
    }
    av_opt_set_int(this->swr, "in_channel_layout", pCodecPar->channel_layout, 0);
    av_opt_set_int(this->swr, "out_channel_layout", pCodecPar->channel_layout, 0);
    av_opt_set_int(this->swr, "in_sample_rate", pCodecPar->sample_rate, 0);
    av_opt_set_int(this->swr, "out_sample_rate", pCodecPar->sample_rate, 0);
    av_opt_set_sample_fmt(this->swr, "in_sample_fmt", static_cast<AVSampleFormat>(pCodecPar->format), 0);
    av_opt_set_sample_fmt(this->swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    if ((swr_init(this->swr)) != 0)
    {
        THROW_RUNTIME_ERROR("Cannot init swr.");
    }

    /* initialize packet, set data to nullptr, let the demuxer fill it */
    if ((this->packet = av_packet_alloc()) == nullptr)
    {
        THROW_RUNTIME_ERROR("Cannot allocate AVPacket.")
    }

    // frame, where the decoded data will be written
    this->frame = av_frame_alloc();
    if (frame == nullptr)
    {
        THROW_RUNTIME_ERROR("Cannot allocate AVFrame.")
    }
}

void FFMpegWrapper::close() noexcept
{
    if (this->swr != nullptr)
    {
        swr_free(&this->swr);
        this->swr = nullptr;
    }

    this->tmpSwrBuf.clear();
    this->tmpSwrBuf.shrink_to_fit();
    av_frame_free(&this->frame);
    av_packet_free(&this->packet);
    avcodec_free_context(&this->codecCtx);
    avformat_close_input(&this->handle);
}

int FFMpegWrapper::decode_packet(int16_t *(&pcm), int &framesToDo)
{
    char errstr[AV_ERROR_MAX_STRING_SIZE];
    int decoded = 0, ret;

    if (this->packet->stream_index == this->audioStreamID)
    {
        if ((ret = avcodec_send_packet(this->codecCtx, this->packet)) < 0)
        {
            switch (ret)
            {
                case AVERROR(EAGAIN):
                    CLOG(LogLevel_t::Error, "avcodec_send_packet() not ready??");
                    break;

                case AVERROR_EOF:
                    CLOG(LogLevel_t::Error, "decoder has been flushed, and no new packets can be sent to it or more than 1 flush packet has been sent");
                    break;

                case AVERROR(EINVAL):
                    CLOG(LogLevel_t::Error, "codec not opened, it is an encoder, or requires flush");
                    break;

                case AVERROR(ENOMEM):
                    CLOG(LogLevel_t::Error, "failed to add packet to internal queue");
                    break;

                default:
                    av_strerror(ret, errstr, sizeof(errstr));
                    CLOG(LogLevel_t::Warning, "error submitting the packet to the decoder (legitimate decoding error): '" << errstr << "'");
                    break;
            }
            return ret;
        }

        /* read all the output frames (in general there may be any number of them) */
        while (ret >= 0)
        {
            ret = avcodec_receive_frame(this->codecCtx, this->frame);
            if (ret == AVERROR(EAGAIN))
            {
                CLOG(LogLevel_t::Debug, "AVERROR(EAGAIN)");
                return ret;
            }
            if (ret == AVERROR_EOF)
            {
                CLOG(LogLevel_t::Debug, "AVERROR_EOF");
                return ret;
            }
            else if (ret < 0)
            {
                av_strerror(ret, errstr, sizeof(errstr));
                CLOG(LogLevel_t::Warning, "legitimate decoding error: '" << errstr << "'");
            }

            //             for (i = 0; i < frame->nb_samples; i++)
            //                 for (ch = 0; ch < this->handle->streams[this->audioStreamID]->codec->channels; ch++)
            //                     fwrite(frame->data[ch] + data_size*i, 1, data_size, outfile);

            /* Some audio decoders decode only part of the packet, and have to be
             * called again with the remainder of the packet data.
             * Sample: fate-suite/lossless-audio/luckynight-partial.shn
             * Also, some decoders might over-read the packet. */
            decoded += this->frame->nb_samples;

            if (this->frame->nb_samples > framesToDo)
            {
                size_t oldNoOfItems = this->tmpSwrBuf.size();
                // not enough space left in the master PCM buffer, render to tmp buffer
                this->tmpSwrBuf.resize(oldNoOfItems + this->frame->nb_samples * this->frame->channels);

                auto *inbuf = this->tmpSwrBuf.data() + oldNoOfItems;
                swr_convert(this->swr, reinterpret_cast<uint8_t **>(&inbuf), this->frame->nb_samples, const_cast<const uint8_t **>(this->frame->extended_data), this->frame->nb_samples);

                // and copy over frames, if there is any space left in the master PCM buffer
                if (framesToDo > 0)
                {
                    frame_t itemsToCopy = framesToDo * this->frame->channels;

                    std::memcpy(pcm, this->tmpSwrBuf.data(), itemsToCopy * sizeof(*pcm));
                    this->tmpSwrBuf.erase(this->tmpSwrBuf.begin(), this->tmpSwrBuf.begin() + itemsToCopy);

                    pcm += itemsToCopy;
                    this->framesAlreadyRendered += framesToDo;
                }
            }
            else
            {
                swr_convert(this->swr, reinterpret_cast<uint8_t **>(&pcm), this->frame->nb_samples, const_cast<const uint8_t **>(this->frame->extended_data), this->frame->nb_samples);

                pcm += this->frame->nb_samples * this->frame->channels;
                this->framesAlreadyRendered += this->frame->nb_samples;
            }

            framesToDo -= this->frame->nb_samples; // could go negative here, i.e. no space left in master PCM buffer
        }
    }
    else
    {
        // dont care
    }

    return decoded;
}

void FFMpegWrapper::render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender)
{
    int framesToDo = framesToRender = std::min(framesToRender, this->getFrames() - this->framesAlreadyRendered);

    // int16 because we told swr to convert everything to int16
    int16_t *pcm = static_cast<int16_t *>(bufferToFill);

    // anything already cached in the temp buffer?
    frame_t itemsToCopy = std::min<frame_t>(this->tmpSwrBuf.size(), Channels * framesToDo);
    if (itemsToCopy > 0)
    {
        std::memcpy(pcm, this->tmpSwrBuf.data(), itemsToCopy * sizeof(*pcm));
        this->tmpSwrBuf.erase(this->tmpSwrBuf.begin(), this->tmpSwrBuf.begin() + itemsToCopy);

        pcm += itemsToCopy;
        framesToDo -= itemsToCopy / Channels;
        this->framesAlreadyRendered += itemsToCopy / Channels;
    }

    // the FFMPEG rendering loop
    bool finished = false;
    while (!this->stopFillBuffer &&
           framesToDo > 0 &&
           pcm < static_cast<int16_t *>(bufferToFill) + this->count)
    {
        /* Read one audio frame from the input file into a temporary packet. */
        int ret = av_read_frame(this->handle, this->packet);
        if (ret < 0)
        {
            if (ret == AVERROR_EOF || avio_feof(this->handle->pb))
            {
                finished = true;
                CLOG(LogLevel_t::Debug, "AVERROR_EOF");
                break;
            }
            if (this->handle->pb != nullptr && this->handle->pb->error)
            {
                CLOG(LogLevel_t::Debug, "pb error: " << this->handle->pb->error);
                break;
            }
        }

        if (this->decode_packet(pcm, framesToDo) == AVERROR_EOF)
        {
            finished = true;
        }
        av_packet_unref(this->packet);
    }

    /* If we are at the end of the file, flush the decoder below, making sure the very last run flushes any cached frames */
    if (finished)
    {
        this->packet->data = nullptr;
        this->packet->size = 0;
        this->decode_packet(pcm, framesToDo);
        av_packet_unref(this->packet);
    }

    this->doAudioNormalization(static_cast<int16_t *>(bufferToFill), framesToRender);
}

frame_t FFMpegWrapper::getFrames() const
{
    frame_t totalFrames = this->fileLen.hasValue ? msToFrames(this->fileLen.Value, this->Format.SampleRate) : 0;

    return totalFrames;
}

void FFMpegWrapper::buildMetadata() noexcept
{
    AVDictionaryEntry *tag = nullptr;

    tag = av_dict_get(this->handle->metadata, "artist", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (tag != nullptr)
    {
        this->Metadata.Artist = tag->value;
    }

    tag = av_dict_get(this->handle->metadata, "title", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (tag != nullptr)
    {
        this->Metadata.Title = tag->value;
    }

    tag = av_dict_get(this->handle->metadata, "album", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (tag != nullptr)
    {
        this->Metadata.Album = tag->value;
    }

    tag = av_dict_get(this->handle->metadata, "composer", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (tag != nullptr)
    {
        this->Metadata.Composer = tag->value;
    }

    tag = av_dict_get(this->handle->metadata, "track", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (tag != nullptr)
    {
        this->Metadata.Track = tag->value;
    }

    tag = av_dict_get(this->handle->metadata, "date", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (tag != nullptr)
    {
        this->Metadata.Year = tag->value;
    }

    tag = av_dict_get(this->handle->metadata, "genre", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (tag != nullptr)
    {
        this->Metadata.Genre = tag->value;
    }

    tag = av_dict_get(this->handle->metadata, "comment", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (tag != nullptr)
    {
        this->Metadata.Comment = tag->value;
    }
}
