#include "AopsfWrapper.h"

#include "CommonExceptions.h"
#include "Config.h"
#include "Common.h"
#include "AtomicWrite.h"

#include <usf.h>
#include <psflib.h>

AopsfWrapper::AopsfWrapper(string filename) : StandardWrapper(filename)
{
    this->initAttr();
}

AopsfWrapper::AopsfWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : StandardWrapper(filename, offset, len)
{
    this->initAttr();
}

void AopsfWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::int16;

    // there will always be 2 channels, if this will be real stereo sound or only mono depends on the game
    this->Format.Channels = 2;
}

AopsfWrapper::~AopsfWrapper ()
{
    this->releaseBuffer();
    this->close();
}

static psf_file_callbacks stdio_callbacks =
{
    "\\/:",
    AopsfWrapper::stdio_fopen,
    AopsfWrapper::stdio_fread,
    AopsfWrapper::stdio_fseek,
    AopsfWrapper::stdio_fclose,
    AopsfWrapper::stdio_ftell
};

void AopsfWrapper::open()
{
    if(this->psfHandle!=nullptr)
    {
        return;
    }

    this->psfVersion = psf_load(this->Filename.c_str(),
                                &stdio_callbacks,
                                0, // psf files might have version 1 or 2, we dont know, so probe for version with 0
                                nullptr,
                                nullptr,
                                &AopsfWrapper::psf_info, // callback function to call for info on this usf file
                                this, // info context
                                1 // yes we want nested info tags, whatever that means
                            );
    
    if(this->psfVersion <= 0)
    {
        THROW_RUNTIME_ERROR("psf_load failed on file \"" << this->Filename << ")\"");
    }
    
    if(this->psfVersion != 1 && this->psfVersion != 2)
    {
        THROW_RUNTIME_ERROR("this is neither a PSF1 nor a PSF2 file \"" << this->Filename << ")\"");
    }
    
    this->psfHandle = new unsigned char[psx_get_state_size(this->psfVersion)];
    memset(this->psfHandle, 0, psx_get_state_size(this->psfVersion));


    this->Format.SampleRate = this->psfVersion == 2 ? 48000 : 44100
    
    int ret = psf_load( this->Filename.c_str(),
                        &stdio_callbacks,
                        this->psfVersion,
                        &AopsfWrapper::psf_loader, // callback function to call on loading this usf file
                        this->psfHandle, // context, i.e. pointer to the struct we place the usf file in
                        nullptr,
                        nullptr,
                        0
                );
    
    if(ret != this->psfVersion)
    {
        THROW_RUNTIME_ERROR("psf_load failed while decoding on file \"" << this->Filename << ")\"");
    }
}

void AopsfWrapper::close() noexcept
{
    if(this->psfHandle != nullptr)
    {
        if(this->psfVersion == 2)
        {
             psf2_stop(this->psfHandle);
        }
        else
        {
            psf_stop(this->psfHandle);
        }
        
        delete [] this->psfHandle;
        this->psfHandle = nullptr;
    }
}

void AopsfWrapper::fillBuffer()
{
    StandardWrapper::fillBuffer(this);
}

void AopsfWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    // TODO: UGLY CAST AHEAD!
    STANDARDWRAPPER_RENDER(int16_t,
                           if(this->psfVersion == 2)
                           {
                                psf2_gen(this->psfHandle, pcm, framesToDoNow);
                           }
                           else
                           {
                                psf_gen(this->psfHandle, pcm, framesToDoNow);
                           }
                          )
}

frame_t AopsfWrapper::getFrames () const
{
    if(this->fileLen.hasValue)
    {
        return msToFrames(this->fileLen.Value, this->Format.SampleRate);
    }
    else
    {
        // 3 minutes by default
        return msToFrames(3*60*1000, this->Format.SampleRate);
    }
}

void AopsfWrapper::buildMetadata() noexcept
{
    // noting to do here, everything is done in psf_info()
}

/// ugly C-helper functions

void * AopsfWrapper::stdio_fopen( const char * path )
{
    return fopen( path, "rb" );
}

size_t AopsfWrapper::stdio_fread( void *p, size_t size, size_t count, void *f )
{
    return fread( p, size, count, (FILE*) f );
}

int AopsfWrapper::stdio_fseek( void * f, int64_t offset, int whence )
{
    return fseek( (FILE*) f, offset, whence );
}

int AopsfWrapper::stdio_fclose( void * f )
{
    return fclose( (FILE*) f );
}

long AopsfWrapper::stdio_ftell( void * f )
{
    return ftell( (FILE*) f );
}

int AopsfWrapper::psf_loader(void * context, const uint8_t * exe, size_t exe_size,
                               const uint8_t * reserved, size_t reserved_size)
{
    if ( exe && exe_size > 0 )
    {
        return -1;
    }

    return usf_upload_section( context, reserved, reserved_size );
}


int AopsfWrapper::psf_info(void * context, const char * name, const char * value)
{
    AopsfWrapper* infoContext = reinterpret_cast<AopsfWrapper*>(context);

    if (iEquals(name, "length"))
    {
//         if(!infoContext->fileLen.hasValue)
//         {
            try
            {
                infoContext->fileLen = parse_time_crap(value);
            }
            catch(runtime_error& e)
            {}
//         }
    }

    else if (iEquals(name, "fade"))
    {
        try
        {
            infoContext->fade_ms = parse_time_crap(value);
        }
        catch(runtime_error& e)
        {}
    }

    else if (iEquals(name, "title"))
    {
        infoContext->Metadata.Title = string(value);
    }

    else if (iEquals(name, "artist"))
    {
        infoContext->Metadata.Artist = infoContext->Metadata.Composer = string(value);
    }

    else if (iEquals(name, "game"))
    {
        infoContext->Metadata.Album = string(value);
    }

    else if (iEquals(name, "year"))
    {
        infoContext->Metadata.Year = string(value);
    }

    else if (iEquals(name, "genre"))
    {
        infoContext->Metadata.Genre = string(value);
    }

    else if (iEquals(name, "track"))
    {
        infoContext->Metadata.Track = string(value);
    }

    else if (iEquals(name, "comment"))
    {
        infoContext->Metadata.Comment = string(value);
    }

    else if (iEquals(name, "_refresh"))
    {
        try
        {
            unsigned long ref = stoul(string(value), nullptr);
            psx_set_refresh(infoContext->psfHandle, ref);
        }
        catch(const invalid_argument& e)
        {
            CLOG(LogLevel::INFO, "psx_set_refresh failed: " << e.what() << " in file '" << this->Filename << "'");
        }
    }
    
    else if (iEquals(name, "utf8"))
    {
        CLOG(LogLevel::INFO, "psf utf8 file found: '" << this->Filename << "'");
    }
    
    else
    {
        CLOG(LogLevel::WARNING, "found unknown tag '" << name << "' value '" << value << "' in file '" << this->Filename << "'");
    }

    return 0;
}
