#include "JackOutput.h"

#include "CommonExceptions.h"
#include "AtomicWrite.h"

#include <iostream>
#include <string>

#include <jack/jack.h>


JackOutput::JackOutput ()
{
}

JackOutput::~JackOutput ()
{
    this->close();
    
    
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
  
  char portName[3+1];
  
  for(unsigned int i=0; i<channels && i <= 99; i++)
  {
    sprintf(portName, 3+1, "%.2d", i);

    // we provide an output port (from the view of jack), i.e. a capture port, i.e. a readable port
    jack_port_t* out_port = jack_port_register (this->handle, portName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    
    if (out_port == nullptr)
    {
      // TODO: throw or just break??
       THROW_RUNTIME_ERROR("no more JACK ports available");
    }
    
    this->playbackPorts.push_back(out_port);
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
    if (pthis->transportState == JackTransportRolling)
    {
        
    }
}

int JackOutput::write (int16_t* buffer, frame_t frames)
{
    if (pthis->transportState == JackTransportRolling)
    {
        
    }
}

int JackOutput::write (int32_t* buffer, frame_t frames)
{
    if (pthis->transportState == JackTransportRolling)
    {
        
    }
}


void JackOutput::connectPorts()
{
    const char** physicalPlaybackPorts = jack_get_ports(this->handle, NULL, NULL, JackPortIsPhysical|JackPortIsInput);

    for(int i=0; physicalPlaybackPorts[i]!=nullptr && i<this->playbackPorts.size(); i++)
    {
      if (jack_connect(this->handle, jack_port_name(this->playbackPorts[i]), physicalPlaybackPorts[i]))
      {
              CLOG(LogLevel::INFO, "cannot connect to port \"" << ports[0] << "\"");
      }
      
      jack_free(physicalPlaybackPorts[i]);
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
    JackOutput *const pthis = static_cast<JackOutput*>(arg);
    
    // TODO: if (!pthis->bufferReady) return;
    
    
    	pthis->transportState = jack_transport_query(pthis->handle, nullptr);

	if (pthis->transportState == JackTransportRolling)
        {    
            for(int i=0; i<pthis->playbackPorts.size(); i++)
            {
                jack_default_audio_sample_t* out = jack_port_get_buffer(pthis->playbackPorts[i], nframes);
                
                for(unsigned int myIdx=i, jackIdx=0; jackIdx<nframes /*&& myIdx<pthis->interleavedProcessedBuffer.size()*/; myIdx+=pthis->currentChannelCount, jackIdx++)
                {
                    out[jackIdx] = pthis->interleavedProcessedBuffer[myIdx];
                }
            }
        }
        else if (pthis->transportState == JackTransportStopped)
        {
            // return 0 in this->write()
        }
        
        
        
        return 0;
}

// we can do non-realtime safe stuff here :D
int JackOutput::onJackBufSizeChanged(jack_nframes_t nframes, void *arg)
{
    this = static_cast<JackOutput*>(arg);
    
    this->jackBufSize = nframes;
    
    // TODO check if buffer consumed, if not, print a warning and discard samples, who cares...
    if(!this->interleavedProcessedBuffer.consumed)
    {
        CLOG(LogLevel::WARNING, "JackBufSize changed, but buffer has not been consumed yet, discarding pending samples");
    }
    
    delete[] this->interleavedProcessedBuffer.buf;
    this->interleavedProcessedBuffer.buf = new (nothrow) jack_default_audio_sample_t[nframes];
    
    // TODO check if alloc successfull
    
    return 0;
}


int onJackSampleRateChanged(jack_nframes_t nframes, void* arg)
{
    this = static_cast<JackOutput*>(arg);
    
    this->jackSampleRate = nframes;
    
    return 0;
}
