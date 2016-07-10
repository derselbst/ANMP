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
  const char *server_name = NULL;
  
        this->handle = jack_client_open (JackOutput::ClientName, options, &status, server_name);
  
        if (this->handle == NULL)
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
    
    if (out_port == NULL)
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
    return this->write<float>(buffer, frames);
}

int JackOutput::write (int16_t* buffer, frame_t frames)
{
    return this->write<int16_t>(buffer, frames);
}

int JackOutput::write (int32_t* buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
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
  // ??? necessary???
    if (jack_deactivate(this->handle))
    {
            THROW_RUNTIME_ERROR("cannot activate client")
    }
}

