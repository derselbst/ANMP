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

  // TODO implement playforever
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
    if(this->wholeSong())
    {
        if(this->data==nullptr)
        {
            this->count = this->getFrames() * this->Format.Channels;
            this->data = new float[this->count];

            // usually this shouldnt block at all
            WAIT(this->futureFillBuffer);
            
            // (pre-)render the first few milliseconds
            this->render(msToFrames(Config::PreRenderTime, this->Format.SampleRate));

            // immediatly start filling the pcm buffer
            this->futureFillBuffer = async(launch::async, &LibGMEWrapper::render, this, 0);

            // allow the render thread to do his work
            this_thread::yield();
        }
    }
    else
    {
        if(this->data==nullptr)
        {
            this->count = Config::FramesToRender*this->Format.Channels;
            this->data = new int16_t[this->count];
        }
        gme_play(this->handle, Config::FramesToRender, static_cast<int16_t*>(this->data));
    }
}

void LibGMEWrapper::render(frame_t framesToRender)
{
    if(framesToRender==0)
    {
        // render rest of file
        framesToRender = this->getFrames()-this->framesAlreadyRendered;
    }
    else
    {
        framesToRender = min(framesToRender, this->getFrames()-this->framesAlreadyRendered);
    }

    int16_t* pcm = static_cast<int16_t*>(this->data);
    pcm += this->framesAlreadyRendered * this->Format.Channels;

    int framesToDoNow;
    while(framesToRender>0 && !this->stopFillBuffer)
    {
        framesToDoNow = (framesToRender/Config::FramesToRender)>0 ? Config::FramesToRender : framesToRender%Config::FramesToRender;

        gme_play(this->handle, framesToDoNow * this->Format.Channels, pcm);
        
        pcm += framesToDoNow * this->Format.Channels;
        this->framesAlreadyRendered += framesToDoNow;

        framesToRender -= framesToDoNow;
    }
}

void LibGMEWrapper::releaseBuffer()
{
    this->stopFillBuffer=true;
    WAIT(this->futureFillBuffer);

    delete [] static_cast<int16_t*>(this->data);
    this->data=nullptr;
    this->count = 0;
    this->framesAlreadyRendered=0;

    this->stopFillBuffer=false;
}

frame_t LibGMEWrapper::getFrames () const
{
    return Config::gmePlayForever ? msToFrames(-1, this->Format.SampleRate) : msToFrames(this->fileLen, this->Format.SampleRate);
}

vector<loop_t> LibGMEWrapper::getLoopArray () const
{
    vector<loop_t> res;

    if(this->wholeSong() && this->info->intro_length!=-1 && this->info->loop_length!=-1)
    {
	loop_t l;
	l.start = msToFrames(this->info->intro_length, this->Format.SampleRate);
	l.stop  = msToFrames(this->info->intro_length + this->info->loop_length, this->Format.SampleRate);
	l.count = 2;
	res.push_back(l);
    }
    return res;
}

// true if we can hold the whole song in memory
bool LibGMEWrapper::wholeSong() const
{
    return !Config::gmePlayForever;
}
