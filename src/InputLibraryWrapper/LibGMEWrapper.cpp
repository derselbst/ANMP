#include "LibGMEWrapper.h"

#include "CommonExceptions.h"
#include "Config.h"
#include "Common.h"
#include "AtomicWrite.h"



// NOTE! for this class we use Song::fileOffset as track offset (i.e. track num) for libgme
LibGMEWrapper::LibGMEWrapper(string filename, size_t offset, size_t len) : Song(filename, offset, len)
{
    this->Format.SampleFormat = SampleFormat_t::int16;

    // there will always be 2 channels, if this will be real stereo sound or only mono depends on the game
    this->Format.Channels = 2;
}

LibGMEWrapper::~LibGMEWrapper ()
{
    this->close();
    this->releaseBuffer();
}

/* Print any warning for most recent emulator action (load, start_track, play) */
void LibGMEWrapper::printWarning( Music_Emu* emu )
{
	const char* warning = gme_warning( emu );
	if ( warning )
        {
            CLOG(LogLevel::WARNING, warning);
        }
}

void LibGMEWrapper::open()
{
  if(this->handle!=nullptr)
  {
    return;
  }
  
  gme_err_t msg = gme_open_file(this->Filename.c_str(), &this->handle, Config::gmeSampleRate);
  if(msg)
  {
      THROW_RUNTIME_ERROR("libgme failed on file \"" << this->Filename << ")\"" << " with message " << msg);
  }
  
  if(this->handle == nullptr)
  {
      THROW_RUNTIME_ERROR("THIS SHOULD NEVER HAPPEN! libgme handle is NULL although no error was reported");
  }
  
  LibGMEWrapper::printWarning(this->handle);
  
  /* Enable most accurate sound emulation */
  gme_enable_accuracy( this->handle, Config::gmeAccurateEmulation );

  /* Add some stereo enhancement */
  gme_set_stereo_depth( this->handle, Config::gmeEchoDepth );
  
  /* Start track and begin fade at 10 seconds */
  msg = gme_start_track( this->handle, this->fileOffset );
  if(msg)
  {
      THROW_RUNTIME_ERROR("libgme failed to set track no. " << this->fileOffset << " for file \"" << this->Filename << "\" with message: " << msg);
  }
  
  this->printWarning( this->handle );
  
  msg = gme_track_info( this->handle, &this->info, this->fileOffset );
  if(msg || this->info == nullptr)
  {
      THROW_RUNTIME_ERROR("libgme failed to retrieve track info for track no. " << this->fileOffset << " for file \"" << this->Filename << "\" with message: " << msg);
  }

  this->fileLen = this->info->length==-1 ? this->fileLen : this->info->length;
  gme_set_fade(this->handle, this->fileLen);
  
  this->Format.SampleRate = Config::gmeSampleRate;
}

void LibGMEWrapper::close()
{
    if(this->handle != nullptr)
    {
        gme_delete(this->handle);
        this->handle = nullptr;
        
        gme_free_info( info );
        this->info = nullptr;
    }
}

void LibGMEWrapper::fillBuffer()
{
    if(this->data==nullptr)
    {
        this->count = Config::FramesToRender*this->Format.Channels;
        this->data = new int16_t[this->count];
    }

    gme_play(this->handle, this->count, static_cast<int16_t*>(this->data));
}

void LibGMEWrapper::releaseBuffer()
{
    delete [] static_cast<int16_t*>(this->data);
    this->data=nullptr;
    this->count = 0;
}

frame_t LibGMEWrapper::getFrames () const
{
    return msToFrames(this->fileLen, this->Format.SampleRate);
}
