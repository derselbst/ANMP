#include "LibSNDWrapper.h"

#include <sndfile.h>

// Constructors/Destructors
//  

LibSNDWrapper::LibSNDWrapper (string filename)
{
  this->Filename = filename;
  memset (&sfinfo, 0, sizeof (sfinfo)) ;
  this->Format.SampleFormat = SampleFormat_t::float32;
  
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
            size_t BUFFER_LEN = this->getFrames() * this->Format.Channels;
            this->data = new float[BUFFER_LEN];
	    
            int readcount = sf_read_float (sndfile, this->data, BUFFER_LEN);
	    
            if(readcount != BUFFER_LEN)
                // TODO: LOG: printf("THIS SHOULD NEVER HAPPEN: only read %d frames, although there are %d frames in the file\n", readcount/sfinfo.channels, sfinfo.frames);

  }
}

void LibSNDWrapper::releaseBuffer()
{
  delete [] static_cast<float*>(this->data);
  this->data=nullptr;  
}

vector<loop_t> LibSNDWrapper::getLoops () const
{
  static vector<loop_t> res;
  
  if(res.empty())
  {
    loop_t l;
    l.start = 0;
    l.end = this->getFrames();
    l.count = 1;
    res.push_back(l);
    
    SF_INSTRUMENT inst;
            int ret = sf_command (sndfile, SFC_GET_INSTRUMENT, &inst, sizeof (inst)) ;
            if(ret == SF_TRUE && inst.loop_count > 0)
            {
	      
	      for (int i=0; i<inst.loop_count; i++)
	      {
		l.start = inst.loops[i].start;
		l.stop  = inst.loops[i].stop;
		l.count = inst.loops[i].count;
		res.push_back(l);
	      }
            }

            // TODO: make sure loop array is sorted correctly
  std::sort (res.begin(), res.end(), myLoopSort);
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

// should sort descendingly
bool myLoopSort(loop_t i,loop_t j)
{
  return (i.end-i.start)>(j.end-j.start);
}
