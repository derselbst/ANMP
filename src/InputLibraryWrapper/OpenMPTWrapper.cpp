#include "OpenMPTWrapper.h"
#include <libopenmpt/libopenmpt.hpp>

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Config.h"

#include <cstring>
#include <utility>
#include <fstream>

OpenMPTWrapper::OpenMPTWrapper(string filename)
: StandardWrapper(std::move(filename))
{
    this->initAttr();
}

OpenMPTWrapper::OpenMPTWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len)
: StandardWrapper(std::move(filename), offset, len)
{
    this->initAttr();
}

void OpenMPTWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::float32;
}

OpenMPTWrapper::~OpenMPTWrapper()
{
    this->releaseBuffer();
    this->close();
}


void OpenMPTWrapper::open()
{
    // avoid multiple calls to open()
    if (this->handle != nullptr)
    {
        return;
    }

    std::ifstream file( this->Filename, std::ios::binary );
    std::unique_ptr<openmpt::module> mod(new openmpt::module(file));

    this->Format.SetVoices(1);
    this->Format.VoiceChannels[0] = 2;
    this->Format.SampleRate = gConfig.ModPlugSampleRate;

    mod->set_repeat_count(std::min(-1, gConfig.overridingGlobalLoopCount - 1));
    mod->set_render_param(openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, 8);

    this->fileLen = static_cast<size_t>(mod->get_duration_seconds() * 1000 + 0.5);

    this->handle = mod.release();
}

void OpenMPTWrapper::close() noexcept
{
    if (this->handle != nullptr)
    {
        delete this->handle;
        this->handle = nullptr;
    }
}

void OpenMPTWrapper::render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender)
{
    STANDARDWRAPPER_RENDER(float,
                           framesToDoNow = this->handle->read_interleaved_stereo(this->Format.SampleRate, framesToDoNow, pcm);
                           if(framesToDoNow == 0)
                           {
                               break;
                           })
}

frame_t OpenMPTWrapper::getFrames() const
{
    return msToFrames(this->fileLen.Value, this->Format.SampleRate);
}

void OpenMPTWrapper::buildMetadata() noexcept
{
    this->Metadata.Title = this->handle->get_metadata("title");
    this->Metadata.Artist = this->handle->get_metadata("artist");
    this->Metadata.Year = this->handle->get_metadata("date");
    this->Metadata.Comment = this->handle->get_metadata("message_raw");
}

