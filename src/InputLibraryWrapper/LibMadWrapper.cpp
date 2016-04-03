#include "LibMadWrapper.h"

#include "Config.h"
#include "Common.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#include <mad.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h> // strerror


LibMadWrapper::LibMadWrapper (string filename, size_t fileOffset, size_t fileLen) : Song(filename, fileOffset, fileLen)
{
    this->Format.SampleFormat = SampleFormat_t::int16;
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
        throw runtime_error("fopen failed, errno: " + string(strerror(errno)) +"\n");
    }

    int fd = fileno(this->infile);
    struct stat stat;
    if (fstat(fd, &stat) == -1 || stat.st_size == 0)
    {
        throw runtime_error(string("Error: ") + __func__ + string(": ") + string("fstat failed for File \"" + this->Filename + ")\""));
    }
    
    this->mpeglen = stat.st_size;

    this->mpegbuf = static_cast<unsigned char*>(mmap(0, this->mpeglen, PROT_READ, MAP_SHARED, fd, 0));
    if (this->mpegbuf == MAP_FAILED)
    {
        throw runtime_error(string("Error: ") + __func__ + string(": ") + string("mmap failed for File \"" + this->Filename + ")\""));
    }

    struct mad_header header;
    mad_header_init(&header);

    mad_stream_init(&this->stream);
    /* load buffer with MPEG audio data */
    mad_stream_buffer(&this->stream, this->mpegbuf, this->mpeglen);

    if(this->numFrames==0)
    {
      int ret;
      // try to find a valid header
        while((ret=mad_header_decode(&header, &this->stream))!=0 && MAD_RECOVERABLE(this->stream.error));
        
        if(ret!=0)
	{
	  throw runtime_error(string("Error: ") + __func__ + string(": ") + string("unable to find a valid frame-header for File \"" + this->Filename + ")\""));
	}
        
        this->Format.Channels = MAD_NCHANNELS(&header);
        this->Format.SampleRate = header.samplerate;

        // no clue what this 32 does
        // stolen from mad_synth_frame() in synth.c
        this->numFrames = 32 * MAD_NSBSAMPLES(&header);

        while(1)
	{
	  if(mad_header_decode(&header, &this->stream)!=0)
	  {
	    if(MAD_RECOVERABLE(this->stream.error))
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
	    throw runtime_error(string("Error: ") + __func__ + string(": ") + string("channelcount varies within File \"" + this->Filename + ")\""));
	  }
	  
	  if(this->Format.SampleRate != header.samplerate)
	  {
	    throw runtime_error(string("Error: ") + __func__ + string(": ") + string("samplerate varies within File \"" + this->Filename + ")\""));
	  }
	  
          this->numFrames += 32 * MAD_NSBSAMPLES(&header);
        }

        // somehow reset libmad stream
        mad_stream_finish(&this->stream);
        mad_stream_init(&this->stream);
        /* load buffer with MPEG audio data */
        mad_stream_buffer(&this->stream, this->mpegbuf, this->mpeglen);
    }
    mad_header_finish(&header);
}

void LibMadWrapper::close()
{
    mad_stream_finish(&this->stream);

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
    if(this->data==nullptr)
    {
        this->count = this->getFrames() * this->Format.Channels;
        this->data = new int16_t[this->count];

        // usually this shouldnt block at all
        WAIT(this->futureFillBuffer);

        // immediatly start filling the pcm buffer
        this->futureFillBuffer = async(launch::async, &LibMadWrapper::asyncFillBuffer, this);

        // give libmad at least a minor headstart
        this_thread::sleep_for (chrono::milliseconds(1));
    }
}

void LibMadWrapper::asyncFillBuffer()
{
    struct mad_frame frame;
    struct mad_synth synth;

    mad_frame_init(&frame);
    mad_synth_init(&synth);

    unsigned int item=0;
    while (!this->stopFillBuffer)
    {
      int ret = mad_frame_decode(&frame, &this->stream);
      
	  if(ret!=0)
	  {
	    if(MAD_RECOVERABLE(this->stream.error))
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

        while (!this->stopFillBuffer && nsamples--) {
            signed int sample;

            /* output sample(s) in 16-bit signed little-endian PCM */

            sample = LibMadWrapper::toInt16Sample(*left_ch++);
            static_cast<int16_t*>(this->data)[item]=sample;
	    
            item++;

            if (this->Format.Channels == 2)
            {
                sample = LibMadWrapper::toInt16Sample(*right_ch++);
                static_cast<int16_t*>(this->data)[item]=sample;

                item++;
            }
        }

        if(item>this->count)
        {
            CLOG(AtomicWrite::LogLevel::ERROR, "THIS SHOULD NEVER HAPPEN: read " << item <<" items but only expected " << this->count << "\n");
            break;
        }

    }

    mad_synth_finish(&synth);
    mad_frame_finish(&frame);
}

void LibMadWrapper::releaseBuffer()
{
    this->stopFillBuffer=true;
    WAIT(this->futureFillBuffer);

    delete [] static_cast<int16_t*>(this->data);
    this->data=nullptr;
    this->count = 0;

    this->stopFillBuffer=false;
}

vector<loop_t> LibMadWrapper::getLoopArray () const
{
    vector<loop_t> res;

    return res;
}

frame_t LibMadWrapper::getFrames () const
{
    return this->numFrames;
}

/* FROM minimad.c
 *
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */
signed int LibMadWrapper::toInt16Sample(mad_fixed_t sample)
{
    /* round */
    sample += (1L << (MAD_F_FRACBITS - 16));

    /* clip */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    /* quantize */
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}
