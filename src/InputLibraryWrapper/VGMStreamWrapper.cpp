#include "VGMStreamWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "Config.h"

#include <utility>

// Constructors/Destructors
//

VGMStreamWrapper::VGMStreamWrapper(string filename)
: StandardWrapper(std::move(filename))
{
}

VGMStreamWrapper::VGMStreamWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len)
: StandardWrapper(std::move(filename), offset, len)
{
}

VGMStreamWrapper::~VGMStreamWrapper()
{
    this->releaseBuffer();
    this->close();
}

void VGMStreamWrapper::open()
{
    int err;
    if (this->handle != nullptr)
    {
        return;
    }

    this->handle = libvgmstream_init();
    if (handle == nullptr)
    {
        THROW_RUNTIME_ERROR("libvgmstream out of memory?");
    }

    libvgmstream_config_t cfg = {0};
    cfg.ignore_loop = false;
    cfg.auto_downmix_channels = false;
    libvgmstream_setup(this->handle, &cfg);

    libstreamfile_t* sf = libstreamfile_open_from_stdio(this->Filename.c_str());
    err = libvgmstream_open_stream(this->handle, sf, 0);
    libstreamfile_close(sf);

    if(err < 0)
    {
        THROW_RUNTIME_ERROR("failed opening \"" << this->Filename << "\"");
    }

    this->Format.SampleRate = this->handle->format->sample_rate;
    // group all available channels to individual stereo voices
    this->Format.ConfigureVoices(this->handle->format->channels, 2);
    auto sfm = this->handle->format->sample_format;
    switch(sfm)
    {
        case LIBVGMSTREAM_SFMT_PCM16:
            this->Format.SampleFormat = SampleFormat_t::int16;
            break;
        case LIBVGMSTREAM_SFMT_PCM32:
            this->Format.SampleFormat = SampleFormat_t::int32;
            break;
        case LIBVGMSTREAM_SFMT_FLOAT:
            this->Format.SampleFormat = SampleFormat_t::float32;
            break;
        case LIBVGMSTREAM_SFMT_PCM24:
        default:
            THROW_RUNTIME_ERROR("THIS SHOULD NEVER HAPPEN: SampleFormat " << (int)sfm << " unknown");
    }

    // hold a copy
    this->fileLen = (this->handle->format->stream_samples * 1000.0) / this->Format.SampleRate;
}

void VGMStreamWrapper::close() noexcept
{
    if (this->handle != nullptr)
    {
        libvgmstream_free(this->handle);
        this->handle = nullptr;
    }
}

void VGMStreamWrapper::render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender)
{
    auto sfm = this->handle->format->sample_format;
    switch(sfm)
    {
        case LIBVGMSTREAM_SFMT_PCM16:
            STANDARDWRAPPER_RENDER(int16_t, if(0 > libvgmstream_render(this->handle)){THROW_RUNTIME_ERROR("libvgmstream_render failed")};if(0 > libvgmstream_fill(this->handle, pcm, framesToDoNow)){THROW_RUNTIME_ERROR("libvgmstream_fill failed")})
            break;
        case LIBVGMSTREAM_SFMT_PCM32:
            STANDARDWRAPPER_RENDER(int32_t, if(0 > libvgmstream_render(this->handle)){THROW_RUNTIME_ERROR("libvgmstream_render failed")};if(0 > libvgmstream_fill(this->handle, pcm, framesToDoNow)){THROW_RUNTIME_ERROR("libvgmstream_fill failed")})
            break;
        case LIBVGMSTREAM_SFMT_FLOAT:
            STANDARDWRAPPER_RENDER(float, if(0 > libvgmstream_render(this->handle)){THROW_RUNTIME_ERROR("libvgmstream_render failed")};if(0 > libvgmstream_fill(this->handle, pcm, framesToDoNow)){THROW_RUNTIME_ERROR("libvgmstream_fill failed")})
            break;
        case LIBVGMSTREAM_SFMT_PCM24:
        default:
            THROW_RUNTIME_ERROR("THIS SHOULD NEVER HAPPEN: SampleFormat " << (int)sfm << " unknown");
    }
}

vector<loop_t> VGMStreamWrapper::getLoopArray() const noexcept
{
    vector<loop_t> res;

    if (this->handle != nullptr && this->handle->format->loop_flag) // does stream contain loop information?
    {
        loop_t l;
        l.start = handle->format->loop_start;
        l.stop = handle->format->loop_end;

        // sanity check, for some reason many super smash bros brawl audio files (e.g. B02.brstm) may specify
        // end of loop past the actual song. in such a case use the last frame as loop.stop in hope that no
        // glitch will be hearable
        if (l.stop > this->getFrames())
        {
            l.stop = this->getFrames();
            CLOG(LogLevel_t::Warning, "\"" << this->Filename << "\" specifies the end of loop past the end of file. The loop was truncated to the last frame available." << std::endl);
        }

        // this will always be an infinite loop
        l.count = 0;
        res.push_back(l);
    }

    return res;
}

frame_t VGMStreamWrapper::getFrames() const
{
    return msToFrames(this->fileLen.Value, this->Format.SampleRate);
}

void VGMStreamWrapper::buildMetadata() noexcept
{
    char title[128];
    libvgmstream_title_t cfg = {0};
    cfg.remove_extension = true;
    cfg.filename = this->Filename.c_str();
    libvgmstream_get_title(this->handle, &cfg, title, sizeof(title));

    char describe[1024];
    libvgmstream_format_describe(this->handle, describe, sizeof(describe));
    this->Metadata.Title = title;
    this->Metadata.Comment = describe;
}
