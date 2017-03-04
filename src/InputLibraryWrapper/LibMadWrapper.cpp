#include "LibMadWrapper.h"

#include "Config.h"
#include "Common.h"

#include "AtomicWrite.h"

#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#include <mad.h>
#include <id3tag.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring> // strerror
#include <cmath> // floor


LibMadWrapper::LibMadWrapper(string filename) : StandardWrapper(filename)
{
    this->initAttr();
}

LibMadWrapper::LibMadWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : StandardWrapper(filename, offset, len)
{
    this->initAttr();
}

void LibMadWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::int32;
}

LibMadWrapper::~LibMadWrapper ()
{
    this->releaseBuffer();
    this->close();
}

int LibMadWrapper::findValidHeader(struct mad_header& header)
{
    int ret;

    while((ret=mad_header_decode(&header, this->stream))!=0 && MAD_RECOVERABLE(this->stream->error))
    {
        if(this->stream->error == MAD_ERROR_LOSTSYNC)
        {
            long tagsize = id3_tag_query(this->stream->this_frame,this->stream->bufend - this->stream->this_frame);
            if(tagsize > 0)
            {
                mad_stream_skip(this->stream, tagsize);
                continue;
            }
        }
    }

    return ret;
}

void LibMadWrapper::open ()
{
    // avoid multiple calls to open()
    if(this->infile!=nullptr)
    {
        return;
    }

    this->infile = fopen(this->Filename.c_str(), "rb");
    if (this->infile == nullptr)
    {
        THROW_RUNTIME_ERROR("fopen failed, errno: " << string(strerror(errno)));
    }

    int fd = fileno(this->infile);
    this->mpeglen = getFileSize(fd);
    this->mpegbuf = static_cast<unsigned char*>(mmap(nullptr, this->mpeglen, PROT_READ, MAP_SHARED, fd, 0));
    if (this->mpegbuf == MAP_FAILED)
    {
        THROW_RUNTIME_ERROR("mmap failed for File \"" << this->Filename << ")\"");
    }


    this->stream = new struct mad_stream;
    mad_stream_init(this->stream);
    /* load buffer with MPEG audio data */
    mad_stream_buffer(this->stream, this->mpegbuf, this->mpeglen);

    // we want to know how many pcm frames there are decoded in this file
    // therefore decode header of every mpeg frame
    // pretty expensive, so only to this once
    if(this->numFrames==0)
    {
        struct mad_header header;
        mad_header_init(&header);

        // try to find a valid header
        int ret = this->findValidHeader(header);
        if(ret!=0)
        {
            // only free the locally used header here, this->stream and this->mpegbuf are freed in LibMadWrapper::close()
            mad_header_finish(&header);

            THROW_RUNTIME_ERROR("unable to find a valid frame-header for File \"" + this->Filename + ")\"");
        }

        // a first valid header is good, but it may contain garbage
        this->Format.Channels = MAD_NCHANNELS(&header);
        this->Format.SampleRate = header.samplerate;
        CLOG(LogLevel_t::Debug, "found a first valid header within File \"" << this->Filename << ")\"\n\tchannels: " << MAD_NCHANNELS(&header) << "\nsrate: " << header.samplerate);

        // no clue what this 32 does
        // stolen from mad_synth_frame() in synth.c
        this->numFrames += 32 * MAD_NSBSAMPLES(&header);

        // try to find a second valid header
        ret = this->findValidHeader(header);
        if(ret==0)
        {
            // better use format infos from this header
            this->Format.Channels = max<int>(MAD_NCHANNELS(&header), this->Format.Channels);
            this->Format.SampleRate = header.samplerate;
            CLOG(LogLevel_t::Debug, "found a second valid header within File \"" << this->Filename << ")\"\n\tchannels: " << MAD_NCHANNELS(&header) << "\nsrate: " << header.samplerate);

            this->numFrames += 32 * MAD_NSBSAMPLES(&header);

            // now lets go on and decode rest of file
            while(1)
            {
                if(mad_header_decode(&header, this->stream)!=0)
                {
                    if(MAD_RECOVERABLE(this->stream->error))
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }

                // sanity checks
                if(this->Format.Channels != MAD_NCHANNELS(&header))
                {
                    CLOG(LogLevel_t::Warning, "channelcount varies (now: " << MAD_NCHANNELS(&header) << ") within File \"" << this->Filename << ")\"");
                }

                if(this->Format.SampleRate != header.samplerate)
                {
                    CLOG(LogLevel_t::Warning, "samplerate varies (now: " << header.samplerate << ") within File \"" << this->Filename << ")\"");
                }

                this->numFrames += 32 * MAD_NSBSAMPLES(&header);
            }
        }
        else
        {
            CLOG(LogLevel_t::Warning, "only one valid header found, probably no valid mp3 File \"" << this->Filename << ")\"");
        }

        // somehow reset libmad stream
        mad_stream_finish(this->stream);
        mad_stream_init(this->stream);
        /* load buffer with MPEG audio data */
        mad_stream_buffer(this->stream, this->mpegbuf, this->mpeglen);

        mad_header_finish(&header);
    }

    this->frame.hasValue=true;
    mad_frame_init(&this->frame.Value);

    this->synth.hasValue=true;
    mad_synth_init(&this->synth.Value);
}

void LibMadWrapper::close() noexcept
{

    if(this->synth.hasValue)
    {
        mad_synth_finish(&this->synth.Value);
        this->synth.hasValue=false;
    }

    if(this->frame.hasValue)
    {
        mad_frame_finish(&this->frame.Value);
        this->frame.hasValue=false;
    }

    if(this->stream != nullptr)
    {
        mad_stream_finish(this->stream);
        delete this->stream;
        this->stream = nullptr;
    }

    if(this->mpegbuf!=nullptr)
    {
        munmap(this->mpegbuf, this->mpeglen);
        this->mpegbuf=nullptr;
        this->mpeglen=0;
    }

    if(this->infile!=nullptr)
    {
        fclose(this->infile);
        this->infile=nullptr;
    }
}

void LibMadWrapper::fillBuffer()
{
    StandardWrapper::fillBuffer(this);
}

void LibMadWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    if(framesToRender==0)
    {
        /* render rest of file */
        framesToRender = this->getFrames()-this->framesAlreadyRendered;
    }
    else
    {
        framesToRender = min(framesToRender, this->getFrames()-this->framesAlreadyRendered);
    }

    int32_t* pcm = static_cast<int32_t*>(bufferToFill);

    // if buffer for whole song: adjusts the position where to start filling "bufferToFill", with respect to already rendered frames
    // if only small buffer: since "this->framesAlreadyRendered" should be multiple of this->count: should do pcm+=0
    pcm += (this->framesAlreadyRendered * this->Format.Channels) % this->count;

    // the outer loop, used for decoding and synthesizing MPEG frames
    while(framesToRender>0 && !this->stopFillBuffer)
    {
        // write back tempbuffer, i.e. frames weve buffered from previous calls to libmad (necessary due to inelegant API of libmad, i.e. cant tell how many frames to render during one call)
        {
            const size_t itemsToCpy = min<size_t>(this->tempBuf.size(), framesToRender*this->Format.Channels);

            memcpy(pcm, this->tempBuf.data(), itemsToCpy*sizeof(int32_t));

            this->tempBuf.erase(this->tempBuf.begin(), this->tempBuf.begin()+itemsToCpy);

            const size_t framesCpyd = itemsToCpy / this->Format.Channels;
            framesToRender -= framesCpyd;
            this->framesAlreadyRendered += framesCpyd;

            // again: adjust position
            pcm += itemsToCpy;
        }

        int framesToDoNow = (framesToRender/gConfig.FramesToRender)>0 ? gConfig.FramesToRender : framesToRender%gConfig.FramesToRender;
        if(framesToDoNow==0)
        {
            continue;
        }
        else if(framesToDoNow < 0)
        {
            CLOG(LogLevel_t::Error, "framesToDoNow negative!!!: " << framesToDoNow);
        }

        int ret = mad_frame_decode(&this->frame.Value, this->stream);
        if(ret!=0)
        {
            if(this->stream->error == MAD_ERROR_LOSTSYNC)
            {
                long tagsize = id3_tag_query(this->stream->this_frame,this->stream->bufend - this->stream->this_frame);
                if(tagsize > 0)
                {
                    mad_stream_skip(this->stream, tagsize);
                    continue;
                }
            }

            string errstr=mad_stream_errorstr(this->stream);

            if(MAD_RECOVERABLE(this->stream->error))
            {
                errstr += " (recoverable)";
                CLOG(LogLevel_t::Info, errstr);
                continue;
            }
            else
            {
                errstr += " (not recoverable)";
                CLOG(LogLevel_t::Warning, errstr);
                break;
            }
        }

        mad_synth_frame(&this->synth.Value, &this->frame.Value);

        /* save PCM samples from synth.pcm */
        /* &synth.pcm->samplerate contains the sampling frequency */

        unsigned short nsamples     = this->synth->pcm.length;
        mad_fixed_t const *left_ch  = this->synth->pcm.samples[0];
        mad_fixed_t const *right_ch = this->synth->pcm.samples[1];

        unsigned int item=0;
        /* audio normalization */
        const float absoluteGain = (numeric_limits<int32_t>::max()) / (numeric_limits<int32_t>::max() * this->gainCorrection);

        for( ;
                !this->stopFillBuffer &&
                framesToDoNow>0 && // frames left during this loop
                framesToRender>0 && // frames left during this call
                nsamples>0; // frames left from libmad
                framesToRender--, nsamples--, framesToDoNow--)
        {
            int32_t sample;

            /* output sample(s) in 24-bit signed little-endian PCM */

            sample = LibMadWrapper::toInt24Sample(*left_ch++);
            sample = gConfig.useAudioNormalization ? floor(sample * absoluteGain) : sample;
            pcm[item++] = sample;

            if (this->Format.Channels == 2) // our buffer is for 2 channels
            {
                if(this->synth.Value.pcm.channels==2) // ...but did mad also decoded for 2 channels?
                {
                    sample = LibMadWrapper::toInt24Sample(*right_ch++);
                    sample = gConfig.useAudioNormalization ? floor(sample * absoluteGain) : sample;
                    pcm[item++] = sample;
                }
                else
                {
                    // what? only one channel in a stereo file? well then: pseudo stereo
                    pcm[item++] = sample;

                    CLOG(LogLevel_t::Warning, "decoded only one channel, though this is a stereo file!");
                }
            }

            this->framesAlreadyRendered++;
        }
        pcm += item/* % this->count*/;

        // "bufferToFill" (i.e. "pcm") seems to be full, drain the rest pcm samples from libmad and temporarily save them
        while (!this->stopFillBuffer && nsamples>0)
        {
            int32_t sample;

            /* output sample(s) in 24-bit signed little-endian PCM */

            sample = LibMadWrapper::toInt24Sample(*left_ch++);
            this->tempBuf.push_back(gConfig.useAudioNormalization ? floor(sample * absoluteGain) : sample);

            if (this->Format.Channels == 2)
            {
                sample = LibMadWrapper::toInt24Sample(*right_ch++);
                this->tempBuf.push_back(gConfig.useAudioNormalization ? floor(sample * absoluteGain) : sample);
            }

            /* DONT do this: this->framesAlreadyRendered++; since we use framesAlreadyRendered as offset for "bufferToFill"*/
            nsamples--;
        }

        if(item>this->count)
        {
            CLOG(LogLevel_t::Error, "THIS SHOULD NEVER HAPPEN: read " << item << " items but only expected " << this->count << "\n");
            break;
        }
    }
}

frame_t LibMadWrapper::getFrames () const
{
    return this->numFrames;
}

void LibMadWrapper::buildMetadata() noexcept
{
    // TODO we have to find ID3 tag in the mpeg file, but this should be done while trying decoding mpeg-frame-headers
    // whenever libmad returns lost_sync error, there might be a id3 tag

    struct id3_file *s = id3_file_open(this->Filename.c_str(), ID3_FILE_MODE_READONLY);

    if(s == nullptr)
    {
        return;
    }

    struct id3_tag *t = id3_file_tag(s);
    if(t == nullptr)
    {
        id3_file_close(s);
        return;
    }

    this->Metadata.Title = id3_get_tag(t, ID3_FRAME_TITLE);
    this->Metadata.Artist = id3_get_tag(t, ID3_FRAME_ARTIST);
    this->Metadata.Album = id3_get_tag(t, ID3_FRAME_ALBUM);
    this->Metadata.Year = id3_get_tag(t, ID3_FRAME_YEAR);
    this->Metadata.Genre = id3_get_tag(t, ID3_FRAME_GENRE);
    this->Metadata.Track = id3_get_tag(t, ID3_FRAME_TRACK);
    this->Metadata.Comment = id3_get_tag(t, ID3_FRAME_COMMENT);

    id3_file_close(s);
}


/* stolen from mpg321
 *
 * Convenience for retrieving already formatted id3 data
 * what parameter is one of
 *  ID3_FRAME_TITLE
 *  ID3_FRAME_ARTIST
 *  ID3_FRAME_ALBUM
 *  ID3_FRAME_YEAR
 *  ID3_FRAME_COMMENT
 *  ID3_FRAME_GENRE
 */
string LibMadWrapper::id3_get_tag (struct id3_tag const *tag, char const *what)
{
    struct id3_frame const *frame = NULL;
    union id3_field const *field = NULL;
    int nstrings;
    int avail;
    int j;
    int tocopy;
    int len;
    char printable [1024];
    id3_ucs4_t const *ucs4 = NULL;
    id3_latin1_t *latin1 = NULL;

    memset (printable, '\0', 1024);
    avail = 1024;
    if (strcmp (what, ID3_FRAME_COMMENT) == 0)
    {
        /*There may be sth wrong. I did not fully understand how to use
            libid3tag for retrieving comments  */
        j=0;
        frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, j++);
        if (!frame) return "";
        ucs4 = id3_field_getfullstring (&frame->fields[3]);
        if (!ucs4) return "";
        latin1 = id3_ucs4_latin1duplicate (ucs4);
        if (!latin1 || strlen(reinterpret_cast<char*>(latin1)) == 0) return "";
        len = strlen(reinterpret_cast<char*>(latin1));
        if (avail > len)
            tocopy = len;
        else
            tocopy = 0;
        if (!tocopy) return "";
        avail-=tocopy;
        strncat (printable, reinterpret_cast<char*>(latin1), tocopy);
        free (latin1);
    }

    else
    {
        frame = id3_tag_findframe (tag, what, 0);
        if (!frame) return "";
        field = &frame->fields[1];
        nstrings = id3_field_getnstrings(field);
        for (j=0; j<nstrings; ++j)
        {
            ucs4 = id3_field_getstrings(field, j);
            if (!ucs4) return "";
            if (strcmp (what, ID3_FRAME_GENRE) == 0)
                ucs4 = id3_genre_name(ucs4);
            latin1 = id3_ucs4_latin1duplicate(ucs4);
            if (!latin1) break;
            len = strlen(reinterpret_cast<char*>(latin1));
            if (avail > len)
                tocopy = len;
            else
                tocopy = 0;
            if (!tocopy) break;
            avail-=tocopy;
            strncat (printable, reinterpret_cast<char*>(latin1), tocopy);
            free (latin1);
        }
    }

    return string(printable);
}

/* FROM minimad.c
 *
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 24 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */
#define SAMPLE_SIZE 24
int32_t LibMadWrapper::toInt24Sample(mad_fixed_t sample)
{
    /* round */
    sample += (1L << (MAD_F_FRACBITS - SAMPLE_SIZE));

    /* clip */
    if (sample >= MAD_F_ONE)
    {
        sample = MAD_F_ONE - 1;
    }
    else if (sample < -MAD_F_ONE)
    {
        sample = -MAD_F_ONE;
    }

    /* quantize, i.e. cut the fractional bits we dont need anymore */
    sample >>= (MAD_F_FRACBITS + 1 - SAMPLE_SIZE);

    /* make sure the 24th bit becomes the msb */
    sample <<= sizeof(mad_fixed_t)*8/*==32bits*/ - SAMPLE_SIZE;

    return sample;
}
