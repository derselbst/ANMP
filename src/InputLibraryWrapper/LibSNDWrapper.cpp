#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#include "LibSNDWrapper.h"
#include "CommonExceptions.h"
#include "Config.h"
#include "Common.h"

// Constructors/Destructors
//

LibSNDWrapper::LibSNDWrapper (string filename, size_t offset) : Song(filename, offset)
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

//
// Methods
//

void LibSNDWrapper::open ()
{
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
    sf_close (this->sndfile);
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
        sf_seek(this->sndfile, this->offset, SEEK_SET);

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
	    
	    if(readcount != this->count) {}
	    // TODO: LOG: printf("THIS SHOULD NEVER HAPPEN: only read %d frames, although there are %d frames in the file\n", readcount/sfinfo.channels, sfinfo.frames);

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
//         l.start = 0;
//         l.stop = this->getFrames();
//         l.count = 1;
//         res.push_back(l);

        SF_INSTRUMENT inst;
        int ret = sf_command (this->sndfile, SFC_GET_INSTRUMENT, &inst, sizeof (inst)) ;
        if(ret == SF_TRUE && inst.loop_count > 0)
        {

            for (int i=0; i<inst.loop_count; i++)
            {
	      
        loop_t l;
                l.start = inst.loops[i].start;
                l.stop  = inst.loops[i].end;
		// COMMENT
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
    return this->sfinfo.frames - this->offset;
}


// Accessor methods
//


// Other methods
//

