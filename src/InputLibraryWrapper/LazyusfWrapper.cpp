#include "LazyusfWrapper.h"

#include "CommonExceptions.h"
#include "Config.h"
#include "Common.h"
#include "AtomicWrite.h"

#include <usf.h>
#include <psflib.h>

LazyusfWrapper::LazyusfWrapper(string filename, size_t offset, size_t len) : Song(filename, offset, len)
{
    this->Format.SampleFormat = SampleFormat_t::int16;

    // there will always be 2 channels, if this will be real stereo sound or only mono depends on the game
    this->Format.Channels = 2;
}

LazyusfWrapper::~LazyusfWrapper ()
{
    this->close();
    this->releaseBuffer();
}

static psf_file_callbacks stdio_callbacks =
{
    "\\/:",
    LazyusfWrapper::stdio_fopen,
    LazyusfWrapper::stdio_fread,
    LazyusfWrapper::stdio_fseek,
    LazyusfWrapper::stdio_fclose,
    LazyusfWrapper::stdio_ftell
};

void LazyusfWrapper::open()
{
  if(this->usfHandle!=nullptr)
  {
    return;
  }
  
    this->usfHandle = new unsigned char[usf_get_state_size()];
    usf_clear(this->usfHandle);

    if ( psf_load( this->Filename.c_str(), &stdio_callbacks, 0x21, &LazyusfWrapper::usf_loader, this->usfHandle, &LazyusfWrapper::usf_info, this, 1 ) <= 0 )
    {
      THROW_RUNTIME_ERROR("psf_load failed on file \"" << this->Filename << ")\"");
    }

    usf_set_compare(this->usfHandle, this->enable_compare);
    usf_set_fifo_full(this->usfHandle, this->enable_fifo_full);

    if(Config::useHle)
    {
        usf_set_hle_audio(this->usfHandle, 1);
    }

    // obtain samplerate
    int32_t srate;
    usf_render(this->usfHandle, 0, 0, &srate);
    // TODO: UGLY CAST AHEAD!
    this->Format.SampleRate = static_cast<unsigned int>(srate);
}

void LazyusfWrapper::close()
{
    if(this->usfHandle != nullptr)
    {
        usf_shutdown(this->usfHandle);
        delete [] this->usfHandle;
        this->usfHandle = nullptr;
    }
}

void LazyusfWrapper::fillBuffer()
{
    if(this->data==nullptr)
    {
        this->count = Config::FramesToRender*this->Format.Channels;
        this->data = new int16_t[this->count];
    }

    // TODO: UGLY CAST AHEAD!
    usf_render(this->usfHandle, static_cast<int16_t*>(this->data), Config::FramesToRender, reinterpret_cast<int32_t*>(&this->Format.SampleRate));
}

void LazyusfWrapper::releaseBuffer()
{
    delete [] static_cast<int16_t*>(this->data);
    this->data=nullptr;
    this->count = 0;
}

frame_t LazyusfWrapper::getFrames () const
{
    return msToFrames(fileLen, this->Format.SampleRate);
}


/// ugly C-helper functions

void * LazyusfWrapper::stdio_fopen( const char * path, const char * mode )
{
    return fopen( path, mode );
}

size_t LazyusfWrapper::stdio_fread( void *p, size_t size, size_t count, void *f )
{
    return fread( p, size, count, (FILE*) f );
}

int LazyusfWrapper::stdio_fseek( void * f, int64_t offset, int whence )
{
    return fseek( (FILE*) f, offset, whence );
}

int LazyusfWrapper::stdio_fclose( void * f )
{
    return fclose( (FILE*) f );
}

long LazyusfWrapper::stdio_ftell( void * f )
{
    return ftell( (FILE*) f );
}

int LazyusfWrapper::usf_loader(void * context, const uint8_t * exe, size_t exe_size,
                               const uint8_t * reserved, size_t reserved_size)
{
    if ( exe && exe_size > 0 ) return -1;

    return usf_upload_section( context, reserved, reserved_size );
}


int LazyusfWrapper::usf_info(void * context, const char * name, const char * value)
{
    LazyusfWrapper* infoContext = reinterpret_cast<LazyusfWrapper*>(context);

    if (iEquals(name, "length"))
        infoContext->fileLen += parse_time_crap(value);
    else if (iEquals(name, "fade"))
        infoContext->fade_ms += parse_time_crap(value);
    else if (iEquals(name, "_enablecompare") && *value)
        infoContext->enable_compare = 1;
    else if (iEquals(name, "_enablefifofull") && *value)
        infoContext->enable_fifo_full = 1;

    return 0;
}
