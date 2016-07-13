#include "JackOutput.h"

#include "CommonExceptions.h"
#include "AtomicWrite.h"

#include <iostream>
#include <string>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include <jack/jack.h>


JackOutput::JackOutput ()
{
}

JackOutput::~JackOutput ()
{
    this->close();
    
    if(this->srcState != nullptr)
    {
    	this->srcState = src_delete(this->srcState);
    }
    
    delete[] this->interleavedProcessedBuffer.buf;
}

//
// Interface Implementation
//
void JackOutput::open()
{
  if(this->handle == nullptr)
  {
  
  jack_status_t status;
  jack_options_t options = JackNullOption;
  
        this->handle = jack_client_open (JackOutput::ClientName, options, &status, nullptr);
  
        if (this->handle == nullptr)
        {
          if (status & JackServerFailed)
          {
                  THROW_RUNTIME_ERROR("Unable to connect to JACK server");
          }
          else
          {  
              THROW_RUNTIME_ERROR("jack_client_open() failed, status: " << status);
          }
        }
        
        if (status & JackServerStarted)
        {
                CLOG(LogLevel::INFO, "JACK server started");
        }
        if (status & JackNameNotUnique)
        {
                this->ClientName = jack_get_client_name(this->handle);
                CLOG(LogLevel::WARNING, "unique name " << this->ClientName << " assigned");
        }
        
        jack_set_process_callback(this->handle, JackOutput::processCallback, this);
        jack_set_sample_rate_callback(this->handle, JackOutput::onJackSampleRateChanged, this);
        jack_on_shutdown(this->handle, JackOutput::onJackShutdown, this);
        jack_set_buffer_size_callback(this->handle, JackOutput::onJackBufSizeChanged, this);
        
        // initially assign the buffer size
        JackOutput::onJackBufSizeChanged(jack_get_buffer_size(this->handle), this);
        
        // initially assign the sample rate
        JackOutput::onJackSampleRateChanged(jack_get_sample_rate(this->handle), this);

  }
}

void JackOutput::init(unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime)
{
  this->stop();
  
  // register "channels" number of input ports
  
  
  if(this->currentChannelCount < channels)
  {
  char portName[3+1];
  	  for(unsigned int i=this->currentChannelCount; i<channels && i <= 99; i++)
	  {
	    snprintf(portName, 3+1, "%.2d", i);
	
	    // we provide an output port (from the view of jack), i.e. a capture port, i.e. a readable port
	    jack_port_t* out_port = jack_port_register (this->handle, portName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	    
	    if (out_port == nullptr)
	    {
	      // TODO: throw or just break??
	       THROW_RUNTIME_ERROR("no more JACK ports available");
	    }
	    
	    this->playbackPorts.push_back(out_port);
	  }
  }
  else if(this->currentChannelCount > channels)
  {
          // assumption that (this->currentChannelCount == this->playbackPorts.size()) may not always be correct
  	  for(int i=this->playbackPorts.size()-1; i>=channels; i--)
	  {
	    int err = jack_port_unregister (this->handle, this->playbackPorts[i]);
	    if (err != 0)
	    {
	    	// ?? what to do ??
	    }
	    
	    this->playbackPorts.pop_back();
	  }
  }
  else
  {
  	// this->currentChannelCount == channels, nothing to do here
  }
  
  
    if(this->srcState != nullptr)
    {
    	this->srcState = src_delete(this->srcState);
    }
    int error;
    this->srcState = src_new(SRC_SINC_MEDIUM_QUALITY, channels, &error);
    if(this->srcState == nullptr)
    {
    	THROW_RUNTIME_ERROR("unable to init libsamplerate (" << src_strerror(error) <<")");
    }
    
    // WOW, WE MADE IT TIL HERE, so update channelcount, srate and sformat
    this->currentChannelCount = channels;
    this->currentSampleFormat = s;
    this->currentSampleRate   = sampleRate;
}

void JackOutput::close()
{
    if (this->handle != nullptr)
    {
        jack_client_close (this->handle);
        this->handle = nullptr;
    }
}

int JackOutput::write (float* buffer, frame_t frames)
{
    if (this->transportState == JackTransportRolling)
    {
    	lock_guard<mutex> lock(this->mtx);
    	
    	const unsigned int Frames = min<int64_t>(frames, this->jackBufSize);
    	const unsigned int Items = Frames * this->currentChannelCount;
        for(unsigned int i = 0; i<Items; i++)
        {
            float& item = this->interleavedProcessedBuffer.buf[i];
            // amplify
            item *= this->volume;
            
            // clip
            if(item > 1.0f)
            {
            	item = 1.0f;
            }
            else if(item < -1.0f)
            {
            	item = -1.0f;
            }
        }
        
        return Frames;
    }
    else
    {
    	return 0;
    }
}

int JackOutput::write (int16_t* buffer, frame_t frames)
{
while( this->interleavedProcessedBuffer.ready && // buffer is ready
      !this->interleavedProcessedBuffer.consumed) // and has not been consumed yet
{
	this_thread::sleep_for(chrono::milliseconds(1));
}
	
    if (this->transportState == JackTransportRolling)
    {
    	// converted_to_float_but_not_resampled buffer
    	float* tempBuf = new float[frames];
    	for(unsigned int i = 0; i<frames*this->currentChannelCount; i++)
        {
            float& item = tempBuf[i];
            // convert that item in buffer[i] to float
            item = buffer[i] / (float)(numeric_limits<int16_t>::max()+1);
            // amplify
            item *= this->volume;
            
            // clip
            if(item > 1.0f)
            {
            	item = 1.0f;
            }
            else if(item < -1.0f)
            {
            	item = -1.0f;
            }
        }
        SRC_DATA srcData;
        srcData.data_in = tempBuf;
        srcData.input_frames = frames;
        
        // this is not the last buffer passed to src
        srcData.end_of_input = 0;
        
        int err;
        {
	    	lock_guard<mutex> lock(this->mtx);
	    	srcData.data_out = this->interleavedProcessedBuffer.buf;
	    	srcData.output_frames = this->jackBufSize;
	    	
	    	// output_sample_rate / input_sample_rate
	    	srcData.src_ratio = this->jackSampleRate / this->currentSampleRate;
	    	
	    	err = src_process (this->srcState, &srcData);
        }
    	if(err != 0 )
    	{
    		CLOG(LogLevel::ERROR, "libsamplerate failed processing (" << src_strerror(err) <<")");
    	}
    	else
    	{
        	this->interleavedProcessedBuffer.ready = true;
    	}
        delete[] tempBuf;
        
        if(srcData.output_frames_gen < srcData.output_frames)
        {
            CLOG(LogLevel::WARNING, "jacks callback buffer has not been filled completely; output_frames_gen: " << srcData.output_frames_gen << "  output_frames: " << srcData.output_frames)
        }
        
        return srcData.input_frames_used;
    }
    else
    {
    	// transport not rolling, nothing could be written
    	return 0;
    }
}

int JackOutput::write (int32_t* buffer, frame_t frames)
{
    if (this->transportState == JackTransportRolling)
    {
        
    }
    else
    {
      return 0;
    }
}


void JackOutput::connectPorts()
{
    const char** physicalPlaybackPorts = jack_get_ports(this->handle, NULL, NULL, JackPortIsPhysical|JackPortIsInput);

    for(unsigned int i=0; physicalPlaybackPorts[i]!=nullptr && i<this->playbackPorts.size(); i++)
    {
      if (jack_connect(this->handle, jack_port_name(this->playbackPorts[i]), physicalPlaybackPorts[i]))
      {
              CLOG(LogLevel::INFO, "cannot connect to port \"" << physicalPlaybackPorts[i] << "\"");
      }
      
      jack_free(const_cast<char*>(physicalPlaybackPorts[i]));
    }
}

void JackOutput::start()
{
    if (jack_activate(this->handle))
    {
            THROW_RUNTIME_ERROR("cannot activate client")
    }
    
    this->connectPorts();
}

void JackOutput::stop()
{
    // nothing ;)
}

int JackOutput::processCallback(jack_nframes_t nframes, void* arg)
{
    JackOutput *const pthis = static_cast<JackOutput*const>(arg);
    
    if (!pthis->interleavedProcessedBuffer.ready)
    {
    	return 0;
    }
    
    	pthis->transportState = jack_transport_query(pthis->handle, nullptr);

	if (pthis->transportState == JackTransportRolling)
        {    
            for(unsigned int i=0; i<pthis->playbackPorts.size(); i++)
            {
                jack_default_audio_sample_t* out = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(pthis->playbackPorts[i], nframes));
                
                for(unsigned int myIdx=i, jackIdx=0; jackIdx<nframes /*&& myIdx<pthis->interleavedProcessedBuffer.size()*/; myIdx+=pthis->currentChannelCount, jackIdx++)
                {
                    out[jackIdx] = pthis->interleavedProcessedBuffer.buf[myIdx];
                }
            }
        }
        else if (pthis->transportState == JackTransportStopped)
        {
            // return 0 in this->write()
        }
        
        pthis->interleavedProcessedBuffer.consumed = true;
        pthis->interleavedProcessedBuffer.ready = false;
        
        return 0;
}

// we can do non-realtime safe stuff here :D
int JackOutput::onJackBufSizeChanged(jack_nframes_t nframes, void *arg)
{
    JackOutput *const pthis = static_cast<JackOutput*const>(arg);
    lock_guard<mutex> lock(pthis->mtx);
    
    pthis->jackBufSize = nframes;
    
    // if buffer not consumed, print a warning and discard samples, who cares...
    if(!pthis->interleavedProcessedBuffer.consumed)
    {
        CLOG(LogLevel::WARNING, "JackBufSize changed, but buffer has not been consumed yet, discarding pending samples");
    }
    
    delete[] pthis->interleavedProcessedBuffer.buf;
    pthis->interleavedProcessedBuffer.buf = new (nothrow) jack_default_audio_sample_t[pthis->jackBufSize * pthis->currentChannelCount];
    
    // TODO check if alloc successfull
    
    return 0;
}


int JackOutput::onJackSampleRateChanged(jack_nframes_t nframes, void* arg)
{
    JackOutput *const pthis = static_cast<JackOutput*const>(arg);
    
    pthis->jackSampleRate = nframes;
    
    return 0;
}

void JackOutput::onJackShutdown(void* arg)
{
}
