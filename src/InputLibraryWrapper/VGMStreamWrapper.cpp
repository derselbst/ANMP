#include "VGMStreamWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "Config.h"

extern "C" {
#include <util.h>

#include <utility>
}
// Constructors/Destructors
//

VGMStreamWrapper::VGMStreamWrapper(string filename)
: StandardWrapper(std::move(filename))
{
    this->initAttr();
}

VGMStreamWrapper::VGMStreamWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len)
: StandardWrapper(std::move(filename), offset, len)
{
    this->initAttr();
}

void VGMStreamWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::int16;
}

VGMStreamWrapper::~VGMStreamWrapper()
{
    this->releaseBuffer();
    this->close();
}

void VGMStreamWrapper::open()
{
    if (this->handle != nullptr)
    {
        return;
    }

    this->handle = init_vgmstream(this->Filename.c_str());

    if (handle == nullptr)
    {
        THROW_RUNTIME_ERROR("failed opening \"" << this->Filename << "\"");
    }

    this->Format.SampleRate = this->handle->sample_rate;
    // group all available channels to individual stereo voices
    this->Format.ConfigureVoices(this->handle->channels, 2);

    // hold a copy
    this->fileLen = (this->handle->num_samples * 1000.0) / this->Format.SampleRate;
}

void VGMStreamWrapper::close() noexcept
{
    if (this->handle != nullptr)
    {
        close_vgmstream(this->handle);
        this->handle = nullptr;
    }
}

void VGMStreamWrapper::fillBuffer()
{
    StandardWrapper<int16_t>::fillBuffer(this);
}

void VGMStreamWrapper::render(pcm_t *bufferToFill, frame_t framesToRender)
{
    STANDARDWRAPPER_RENDER(int16_t, render_vgmstream(pcm, framesToDoNow, this->handle))

    this->doAudioNormalization(static_cast<int16_t *>(bufferToFill), framesToRender);
}

vector<loop_t> VGMStreamWrapper::getLoopArray() const noexcept
{
    vector<loop_t> res;

    if (this->handle != nullptr && this->handle->loop_flag) // does stream contain loop information?
    {
        loop_t l;
        l.start = handle->loop_start_sample;
        l.stop = handle->loop_end_sample;

        // sanity check, for some reason many super smash bros brawl audio files (e.g. B02.brstm) may specify
        // end of loop past the actual song. in such a case use the last frame as loop.stop in hope that no
        // glitch will be hearable
        if (l.stop > this->getFrames())
        {
            l.stop = this->getFrames();
            CLOG(LogLevel_t::Warning, "\"" << this->Filename << "\" specifies the end of loop past the end of file. The loop was truncated to the last frame available." << endl)
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
    // as of 07-04-2016 vgmstream doesnt seem to provide any useable metadata, so we cant do anything here
}
