#include "LibMadWrapper.h"

#include "Config.h"
#include "Common.h"

#include "AtomicWrite.h"

#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#include <mad.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h> // strerror


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
    struct stat stat;
    if (fstat(fd, &stat) == -1 || stat.st_size == 0)
    {
      THROW_RUNTIME_ERROR("fstat failed for File \"" << this->Filename << ")\"");
    }
    
    this->mpeglen = stat.st_size;

    this->mpegbuf = static_cast<unsigned char*>(mmap(0, this->mpeglen, PROT_READ, MAP_SHARED, fd, 0));
    if (this->mpegbuf == MAP_FAILED)
    {
      THROW_RUNTIME_ERROR("mmap failed for File \"" << this->Filename << ")\"");
    }

    struct mad_header header;
    mad_header_init(&header);

    this->stream = new struct mad_stream;
    mad_stream_init(this->stream);
    /* load buffer with MPEG audio data */
    mad_stream_buffer(this->stream, this->mpegbuf, this->mpeglen);

    if(this->numFrames==0)
    {
      int ret;
      // try to find a valid header
        while((ret=mad_header_decode(&header, this->stream))!=0 && MAD_RECOVERABLE(this->stream->error));
        
        if(ret!=0)
	{
	  THROW_RUNTIME_ERROR("unable to find a valid frame-header for File \"" + this->Filename + ")\"");
	}
        
        this->Format.Channels = MAD_NCHANNELS(&header);
        this->Format.SampleRate = header.samplerate;

        // no clue what this 32 does
        // stolen from mad_synth_frame() in synth.c
        this->numFrames = 32 * MAD_NSBSAMPLES(&header);

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
	    THROW_RUNTIME_ERROR("channelcount varies within File \"" << this->Filename << ")\"");
	  }
	  
	  if(this->Format.SampleRate != header.samplerate)
	  {
	    THROW_RUNTIME_ERROR("samplerate varies within File \"" << this->Filename << ")\"");
	  }
	  
          this->numFrames += 32 * MAD_NSBSAMPLES(&header);
        }

        // somehow reset libmad stream
        mad_stream_finish(this->stream);
        mad_stream_init(this->stream);
        /* load buffer with MPEG audio data */
        mad_stream_buffer(this->stream, this->mpegbuf, this->mpeglen);
    }
    mad_header_finish(&header);
}

void LibMadWrapper::close()
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
    StandardWrapper<int32_t>::fillBuffer(this);
}

void LibMadWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
  // TODO per frame rendering not yet supported
  if(framesToRender!=0)
  {
    return;
  }
  
    struct mad_frame frame;
    struct mad_synth synth;

    mad_frame_init(&frame);
    mad_synth_init(&synth);

    unsigned int item=0;
    while (!this->stopFillBuffer)
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
	  
        mad_synth_frame(&synth, &frame);

        /* save PCM samples from synth.pcm */
        struct mad_pcm* pcm = &synth.pcm;
        mad_fixed_t const *left_ch, *right_ch;

        /* pcm->samplerate contains the sampling frequency */

        unsigned short& nsamples  = pcm->length;
        left_ch   = pcm->samples[0];
        right_ch  = pcm->samples[1];

        while (!this->stopFillBuffer && nsamples--)
	{
            signed int sample;

            /* output sample(s) in 16-bit signed little-endian PCM */

            sample = LibMadWrapper::toInt16Sample(*left_ch++);
            static_cast<int32_t*>(bufferToFill)[item]=sample;
	    
            item++;

            if (this->Format.Channels == 2)
            {
                sample = LibMadWrapper::toInt16Sample(*right_ch++);
                static_cast<int32_t*>(bufferToFill)[item]=sample;

                item++;
            }
        }

        if(item>this->count)
        {
            CLOG(LogLevel::ERROR, "THIS SHOULD NEVER HAPPEN: read " << item << " items but only expected " << this->count << "\n");
            break;
        }

    }

    mad_synth_finish(&synth);
    mad_frame_finish(&frame);
}

void LibMadWrapper::releaseBuffer()
{
    StandardWrapper<int32_t>::releaseBuffer();
}


frame_t LibMadWrapper::getFrames () const
{
    return this->numFrames;
}

void LibMadWrapper::buildMetadata()
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
int32_t LibMadWrapper::toInt16Sample(mad_fixed_t sample)
{
    /* round */
    sample += (1L << (MAD_F_FRACBITS - SAMPLE_SIZE));

    /* clip */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    /* quantize, i.e. cut the fractional bits we dont need anymore */
    sample >>= (MAD_F_FRACBITS + 1 - SAMPLE_SIZE);
    
    /* make sure the 24th bit becomes the msb */
    sample <<= sizeof(mad_fixed_t)*8/*==32*/ - SAMPLE_SIZE;
    
    return sample;
}
