#include "LazyusfWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Config.h"

#include <psflib.h>
#include <usf.h>

#include <utility>

using namespace std;

LazyusfWrapper::LazyusfWrapper(string filename)
: StandardWrapper(std::move(filename))
{
    this->initAttr();
}

LazyusfWrapper::LazyusfWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len)
: StandardWrapper(std::move(filename), offset, len)
{
    this->initAttr();
}

void LazyusfWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::int16;

    this->Format.SetVoices(1);
    // there will always be 2 channels, if this will be real stereo sound or only mono depends on the game
    this->Format.VoiceChannels[0] = 2;
}

LazyusfWrapper::~LazyusfWrapper()
{
    this->releaseBuffer();
    this->close();
}

static psf_file_callbacks stdio_callbacks =
{
"\\/:",
NULL,
LazyusfWrapper::stdio_fopen,
LazyusfWrapper::stdio_fread,
LazyusfWrapper::stdio_fseek,
LazyusfWrapper::stdio_fclose,
LazyusfWrapper::stdio_ftell};

void LazyusfWrapper::open()
{
    if (this->usfHandle != nullptr)
    {
        return;
    }

    this->usfHandle = new unsigned char[usf_get_state_size()];
    usf_clear(this->usfHandle);

    if (psf_load(this->Filename.c_str(),
                 &stdio_callbacks,
                 0x21, // usf files are psf files with version 0x21
                 &LazyusfWrapper::usf_loader, // callback function to call on loading this usf file
                 this->usfHandle, // context, i.e. pointer to the struct we place the usf file in
                 &LazyusfWrapper::usf_info, // callback function to call for info on this usf file
                 this, // info context
                 1 // yes we want nested info tags, whatever that means
                 ) <= 0)
    {
        THROW_RUNTIME_ERROR("psf_load failed on file \"" << this->Filename << ")\"");
    }

    usf_set_compare(this->usfHandle, this->enable_compare);
    usf_set_fifo_full(this->usfHandle, this->enable_fifo_full);

    if (gConfig.useHle)
    {
        usf_set_hle_audio(this->usfHandle, 1);
    }

    if (this->Format.SampleRate == 0)
    {
        // obtain samplerate
        int32_t srate;
        usf_render(this->usfHandle, 0, 0, &srate);
        // TODO: UGLY CAST AHEAD!
        this->Format.SampleRate = static_cast<unsigned int>(srate);
    }
}

void LazyusfWrapper::close() noexcept
{
    if (this->usfHandle != nullptr)
    {
        usf_shutdown(this->usfHandle);
        delete[] this->usfHandle;
        this->usfHandle = nullptr;
    }
}

void LazyusfWrapper::render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender)
{
    int32_t rate = this->Format.SampleRate;
STANDARDWRAPPER_RENDER(int16_t,
                       usf_render(this->usfHandle, pcm, framesToDoNow, &rate))

}

frame_t LazyusfWrapper::getFrames() const
{
    if (this->fileLen.hasValue)
    {
        return msToFrames(this->fileLen.Value, this->Format.SampleRate);
    }
    else
    {
        // 3 minutes by default
        return msToFrames(3 * 60 * 1000, this->Format.SampleRate);
    }
}

void LazyusfWrapper::buildMetadata() noexcept
{
    // noting to do here, everything is done in usf_info()
}

/// ugly C-helper functions

void *LazyusfWrapper::stdio_fopen(void *ctx, const char *path)
{
    return fopen(path, "rb");
}

size_t LazyusfWrapper::stdio_fread(void *p, size_t size, size_t count, void *f)
{
    return fread(p, size, count, (FILE *)f);
}

int LazyusfWrapper::stdio_fseek(void *f, int64_t offset, int whence)
{
    return fseek((FILE *)f, offset, whence);
}

int LazyusfWrapper::stdio_fclose(void *f)
{
    return fclose((FILE *)f);
}

long LazyusfWrapper::stdio_ftell(void *f)
{
    return ftell((FILE *)f);
}

int LazyusfWrapper::usf_loader(void *context, const uint8_t *exe, size_t exe_size, const uint8_t *reserved, size_t reserved_size)
{
    if (exe && exe_size > 0)
    {
        return -1;
    }

    return usf_upload_section(context, reserved, reserved_size);
}


int LazyusfWrapper::usf_info(void *context, const char *name, const char *value)
{
    LazyusfWrapper *infoContext = reinterpret_cast<LazyusfWrapper *>(context);

    if (iEquals(name, "length"))
    {
        if (!infoContext->fileLen.hasValue) // we might get multiple length tags (e.g. from usflib), only use the first one
        {
            try
            {
                infoContext->fileLen = parse_time_crap(value);
            }
            catch (runtime_error &e)
            {
            }
        }
    }

    else if (iEquals(name, "fade"))
    {
        try
        {
            infoContext->fade_ms = parse_time_crap(value);
        }
        catch (runtime_error &e)
        {
        }
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

    else if (iEquals(name, "_enablecompare") && *value)
    {
        infoContext->enable_compare = 1;
    }

    else if (iEquals(name, "_enablefifofull") && *value)
    {
        infoContext->enable_fifo_full = 1;
    }

    return 0;
}
