#include <cstring>

#include "LibSNDWrapper.h"
#include "CommonExceptions.h"

// Constructors/Destructors
//  

LibSNDWrapper::LibSNDWrapper (string filename, size_t offset)
{
  this->Filename = filename;
  memset (&sfinfo, 0, sizeof (sfinfo)) ;
  this->Format.SampleFormat = SampleFormat_t::float32;
  this->offset = offset;
  
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
	      throw runtime_error(string(sf_strerror (NULL)));
            };

            if (sfinfo.channels < 1 || sfinfo.channels >= 6)
            {
	      throw runtime_error("Error : channels = " + to_string(sfinfo.channels));
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
    sf_seek(this->sndfile, this->offset, SEEK_SET);
    
            size_t BufferLen = this->getFrames() * this->Format.Channels;
	    BufferLen -= this->offset * this->Format.Channels;
            this->data = new float[BufferLen];
	    
            int readcount = sf_read_float (this->sndfile, static_cast<float*>(this->data), BufferLen);
	    
            if(readcount != BufferLen){}
                // TODO: LOG: printf("THIS SHOULD NEVER HAPPEN: only read %d frames, although there are %d frames in the file\n", readcount/sfinfo.channels, sfinfo.frames);
	    
	    this->count = readcount;

  }
}

void LibSNDWrapper::releaseBuffer()
{
  delete [] static_cast<float*>(this->data);
  this->data=nullptr;
  this->count = 0;
}

vector<loop_t> LibSNDWrapper::getLoops () const
{
  static vector<loop_t> res;
  
  if(res.empty())
  {
    loop_t l;
    l.start = 0;
    l.stop = this->getFrames();
    l.count = 1;
    res.push_back(l);
    
    SF_INSTRUMENT inst;
            int ret = sf_command (this->sndfile, SFC_GET_INSTRUMENT, &inst, sizeof (inst)) ;
            if(ret == SF_TRUE && inst.loop_count > 0)
            {
	      
	      for (int i=0; i<inst.loop_count; i++)
	      {
		l.start = inst.loops[i].start;
		l.stop  = inst.loops[i].end;
		l.count = inst.loops[i].count;
		res.push_back(l);
	      }
            }
  }
   return res;
}

unsigned int LibSNDWrapper::getFrames () const
{
  return this->sfinfo.frames;
}


// Accessor methods
//  


// Other methods
//  

