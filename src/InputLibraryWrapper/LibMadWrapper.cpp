#include "LibMadWrapper.h"

#include "Config.h"
#include "Common.h"

#include "AtomicWrite.h"

#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#include <mad.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring> // strerror


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

    if(this->numFrames==0)
    {
        struct mad_header header;
        mad_header_init(&header);
    
        int ret;
        // try to find a valid header
        while((ret=mad_header_decode(&header, this->stream))!=0 && MAD_RECOVERABLE(this->stream->error));

        if(ret!=0)
        {
            // only free the locally used header here, this->stream and this->mpegbuf are freed in LibMadWrapper::close()
            mad_header_finish(&header);
            
            THROW_RUNTIME_ERROR("unable to find a valid frame-header for File \"" + this->Filename + ")\"");
        }

        // a first valid header is good, but it may contain garbage
        this->Format.Channels = MAD_NCHANNELS(&header);
        this->Format.SampleRate = header.samplerate;
        CLOG(LogLevel::DEBUG, "found valid header within File \"" << this->Filename << ")\"\n\tchannels: " << MAD_NCHANNELS(&header) << "\nsrate: " << header.samplerate);

        // no clue what this 32 does
        // stolen from mad_synth_frame() in synth.c
        this->numFrames = 32 * MAD_NSBSAMPLES(&header);

        // try to find a second valid header
        while((ret=mad_header_decode(&header, this->stream))!=0 && MAD_RECOVERABLE(this->stream->error));
        if(ret==0)
        {
            // better use format infos from this header
            this->Format.Channels = MAD_NCHANNELS(&header);
            this->Format.SampleRate = header.samplerate;
            
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
                    CLOG(LogLevel::WARNING, "channelcount varies (now: " << MAD_NCHANNELS(&header) << ") within File \"" << this->Filename << ")\"");
                }

                if(this->Format.SampleRate != header.samplerate)
                {
                    CLOG(LogLevel::WARNING, "samplerate varies (now: " << header.samplerate << ") within File \"" << this->Filename << ")\"");
                }

                this->numFrames += 32 * MAD_NSBSAMPLES(&header);
            }
        }
        else
        {
            CLOG(LogLevel::WARNING, "only one valid header found, probably no valid mp3 File \"" << this->Filename << ")\"");
        }
        
        // somehow reset libmad stream
        mad_stream_finish(this->stream);
        mad_stream_init(this->stream);
        /* load buffer with MPEG audio data */
        mad_stream_buffer(this->stream, this->mpegbuf, this->mpeglen);
        
        mad_header_finish(&header);
    }
}

void LibMadWrapper::close() noexcept
{
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
    CLOG(LogLevel::DEBUG, "Calling LibMadWrapper::render\n\tframesAlreadyRendered: " << this->framesAlreadyRendered
    << "\tcount: " << this->count
    << "\tframesToRender: " << framesToRender);
    
    if(framesToRender==0)
    {
        /* render rest of file */
        framesToRender = this->getFrames()-this->framesAlreadyRendered;
    }
    else
    {
        framesToRender = min(framesToRender, this->getFrames()-this->framesAlreadyRendered);
    }
    fesetround(FE_TONEAREST);
    CLOG(LogLevel::DEBUG, "\ti.e. framesToRender: " << framesToRender);

    int32_t* pcm = static_cast<int32_t*>(bufferToFill);
    
    // if buffer for whole song: adjusts the position where to start filling "bufferToFill", with respect to already rendered frames
    // if only small buffer: since "this->framesAlreadyRendered" should be multiple of this->count: should do pcm+=0
    pcm += (this->framesAlreadyRendered * this->Format.Channels) % this->count;
    CLOG(LogLevel::DEBUG, "\t(this->framesAlreadyRendered * this->Format.Channels): " << (this->framesAlreadyRendered * this->Format.Channels));
    CLOG(LogLevel::DEBUG, "\t+= : " << (this->framesAlreadyRendered * this->Format.Channels) % this->count);

    // write back tempbuffer, i.e. frames weve buffered from previous calls to libmad (necessary due to inelegant API of libmad, i.e. cant tell how many frames to render during one call)
    {
        CLOG(LogLevel::DEBUG, "\titems buffered: " << this->tempBuf.size());
        const size_t itemsToCpy = min<size_t>(this->tempBuf.size(), framesToRender*this->Format.Channels);
        CLOG(LogLevel::DEBUG, "\titemsToCpy: " << itemsToCpy);
        
        memcpy(pcm, this->tempBuf.data(), itemsToCpy*sizeof(int32_t));
        
        this->tempBuf.erase(this->tempBuf.begin(), this->tempBuf.begin()+itemsToCpy);
        
        const size_t framesCpyd = itemsToCpy / this->Format.Channels;
        framesToRender -= framesCpyd;
        this->framesAlreadyRendered += framesCpyd;
        CLOG(LogLevel::DEBUG, "\tframesToRender: " << framesToRender);
    
        // again: adjust position
        pcm += itemsToCpy;
    }
    
    struct mad_frame frame;
    struct mad_synth synth;

    mad_frame_init(&frame);
    mad_synth_init(&synth);

    // the outer loop, used for decoding and synthesizing MPEG frames
    while(framesToRender>0 && !this->stopFillBuffer)
    {
        int ret = mad_frame_decode(&frame, this->stream);
        if(ret!=0)
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
        
        int framesToDoNow = (framesToRender/Config::FramesToRender)>0 ? Config::FramesToRender : framesToRender%Config::FramesToRender;
        CLOG(LogLevel::DEBUG, "\tframesToDoNow: " << framesToDoNow);
        
        mad_synth_frame(&synth, &frame);
        
        /* save PCM samples from synth.pcm */
        /* &synth.pcm->samplerate contains the sampling frequency */

        unsigned short nsamples     = synth.pcm.length;
        mad_fixed_t const *left_ch  = synth.pcm.samples[0];
        mad_fixed_t const *right_ch = synth.pcm.samples[1];
        CLOG(LogLevel::DEBUG, "\tnsamples: " << nsamples);
        
        unsigned int item=0;
        /* audio normalization */
        /*const*/ float absoluteGain = (numeric_limits<int32_t>::max()) / (numeric_limits<int32_t>::max() * this->gainCorrection);
        /* reduce risk of clipping, remove that when using true sample peak */
        absoluteGain -= 0.01;
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
            pcm[item++] = Config::useAudioNormalization ? lrint(sample * absoluteGain) : sample;

            if (this->Format.Channels == 2)
            {
                sample = LibMadWrapper::toInt24Sample(*right_ch++);
                pcm[item++] = Config::useAudioNormalization ? lrint(sample * absoluteGain) : sample;
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
            this->tempBuf.push_back(Config::useAudioNormalization ? lrint(sample * absoluteGain) : sample);

            if (this->Format.Channels == 2)
            {
                sample = LibMadWrapper::toInt24Sample(*right_ch++);
                this->tempBuf.push_back(Config::useAudioNormalization ? lrint(sample * absoluteGain) : sample);
            }
            
            /* DONT do this: this->framesAlreadyRendered++; since we use framesAlreadyRendered as offset for "bufferToFill"*/
            nsamples--;
        }
        CLOG(LogLevel::DEBUG, "---------------");
        CLOG(LogLevel::DEBUG, "\titem: " << item);
        CLOG(LogLevel::DEBUG, "\tnsamples: " << nsamples);
        CLOG(LogLevel::DEBUG, "\tframesToDoNow: " << framesToDoNow);
        CLOG(LogLevel::DEBUG, "\tframesToRender: " << framesToRender);
        CLOG(LogLevel::DEBUG, "-----while-----\n");
        
        if(item>this->count)
        {
            CLOG(LogLevel::ERROR, "THIS SHOULD NEVER HAPPEN: read " << item << " items but only expected " << this->count << "\n");
            break;
        }
    }
    mad_synth_finish(&synth);
    mad_frame_finish(&frame);
}

frame_t LibMadWrapper::getFrames () const
{
    return this->numFrames;
}

void LibMadWrapper::buildMetadata() noexcept
{
    // TODO we have to find ID3 tag in the mpeg file, but this should be done while trying decoding mpeg-frame-headers
    // whenever libmad returns lost_sync error, there might be a id3 tag
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
