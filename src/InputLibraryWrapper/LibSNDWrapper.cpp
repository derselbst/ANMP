#include "LibSNDWrapper.h"

#include "Config.h"
#include "Common.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>


LibSNDWrapper::LibSNDWrapper (string filename, size_t fileOffset, size_t fileLen) : Song(filename, fileOffset, fileLen)
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
        throw runtime_error(string("Error: ") + __func__ + string(": ") + string(sf_strerror (NULL)) + " (in File \"" + this->Filename + ")\"");
    };

    if (sfinfo.channels < 1 || sfinfo.channels >= 6)
    {
        throw runtime_error(string("Error: ") + __func__ + string(": channels == ") + to_string(sfinfo.channels));
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
        this->count = this->getFrames() * this->Format.Channels;
        this->data = new float[this->count];

        // usually this shouldnt block at all
        WAIT(this->futureFillBuffer);

        // immediatly start filling the pcm buffer
        this->futureFillBuffer = async(launch::async, &LibSNDWrapper::asyncFillBuffer, this);

        // give libsnd at least a minor headstart
        this_thread::sleep_for (chrono::milliseconds(1));
    }
}

void LibSNDWrapper::asyncFillBuffer()
{
    sf_seek(this->sndfile, msToFrames(this->fileOffset, this->Format.SampleRate), SEEK_SET);

    int readcount=0;
    unsigned int itemsToRender = Config::FramesToRender*this->Format.Channels;

    int fullFillCount = this->count/itemsToRender;
    for(int i = 0; i<fullFillCount && !this->stopFillBuffer; i++)
    {
        readcount += sf_read_float(this->sndfile, static_cast<float*>(this->data)+i*itemsToRender, itemsToRender);
    }

    if(!this->stopFillBuffer)
    {
        unsigned int lastItemsToRender = this->count % itemsToRender;
        readcount += sf_read_float(this->sndfile, static_cast<float*>(this->data)+fullFillCount*itemsToRender, lastItemsToRender);

        if(readcount != this->count)
	{
        CLOG(AtomicWrite::LogLevel::ERROR, "THIS SHOULD NEVER HAPPEN: only read " << readcount/sfinfo.channels << " frames, although there are " << sfinfo.frames << " frames in the file\n");
	}
        //         this->count = readcount;
    }
}

void LibSNDWrapper::releaseBuffer()
{
    this->stopFillBuffer=true;
    WAIT(this->futureFillBuffer);

    delete [] static_cast<float*>(this->data);
    this->data=nullptr;
    this->count = 0;

    this->stopFillBuffer=false;
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
