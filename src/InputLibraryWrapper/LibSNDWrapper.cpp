#include "LibSNDWrapper.h"

#include "Config.h"
#include "Common.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>


LibSNDWrapper::LibSNDWrapper (string filename, size_t fileOffset, size_t fileLen) : StandardWrapper(filename, fileOffset, fileLen)
{
    this->Format.SampleFormat = SampleFormat_t::float32;
    memset (&sfinfo, 0, sizeof (sfinfo)) ;
    // TODO: just for sure: check whether other instance vars are init properly
}

LibSNDWrapper::~LibSNDWrapper ()
{
    this->releaseBuffer();
    this->close();
}


void LibSNDWrapper::open ()
{
  // avoid multiple calls to open()
  if(this->sndfile!=nullptr)
  {
    return;
  }
  
    if (!(this->sndfile = sf_open (this->Filename.c_str(), SFM_READ, &sfinfo)))
    {
      THROW_RUNTIME_ERROR(sf_strerror (NULL) << " (in File \"" << this->Filename << ")\"");
    };

    if (sfinfo.channels < 1 || sfinfo.channels >= 6)
    {
      THROW_RUNTIME_ERROR("channels == " << sfinfo.channels);
    };

    this->Format.Channels = sfinfo.channels;
    this->Format.SampleRate = sfinfo.samplerate;
}

void LibSNDWrapper::close()
{
  if(this->sndfile!=nullptr)
  {
    sf_close (this->sndfile);
    this->sndfile=nullptr;
  }
  memset (&sfinfo, 0, sizeof (sfinfo)) ;
}

void LibSNDWrapper::fillBuffer()
{
    if(this->data==nullptr)
    {
        sf_seek(this->sndfile, msToFrames(this->fileOffset, this->Format.SampleRate), SEEK_SET);
        StandardWrapper<float>::fillBuffer(this);
    }
}

void LibSNDWrapper::render(frame_t framesToRender)
{
    STANDARDWRAPPER_RENDER(float, sf_read_float(this->sndfile, pcm, framesToDoNow*this->Format.Channels))
}

void LibSNDWrapper::releaseBuffer()
{
    StandardWrapper<float>::releaseBuffer();
}

vector<loop_t> LibSNDWrapper::getLoopArray () const
{
    vector<loop_t> res;

    if(res.empty())
    {
        SF_INSTRUMENT inst;
        int ret = sf_command (this->sndfile, SFC_GET_INSTRUMENT, &inst, sizeof (inst)) ;
        if(ret == SF_TRUE && inst.loop_count > 0)
        {

            for (int i=0; i<inst.loop_count; i++)
            {
                loop_t l;
                l.start = inst.loops[i].start;
                l.stop  = inst.loops[i].end;

                // WARNING: AGAINST RIFF SPEC ahead!!!
                // quoting RIFFNEW.pdf: "dwEnd: Specifies the endpoint of the loop
                // in samples (this sample will also be played)."
                // however (nearly) every piece of software out there ignores that and
                // specifies the sample excluded from the loop
                // THUS: submit to peer pressure
                l.stop -= 1;

                l.count = inst.loops[i].count;
                res.push_back(l);
            }
        }
    }
    return res;
}

frame_t LibSNDWrapper::getFrames () const
{
    int framesAvail = this->sfinfo.frames - msToFrames(this->fileOffset, this->Format.SampleRate);

    if(framesAvail < 0)
    {
      framesAvail=0;
    }
    
    size_t totalFrames = this->fileLen==0 ? framesAvail : msToFrames(this->fileLen, this->Format.SampleRate);

    if(totalFrames > framesAvail)
    {
        totalFrames = framesAvail;
    }

    return totalFrames;
}
