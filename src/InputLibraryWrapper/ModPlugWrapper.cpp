#include "ModPlugWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>

ModPlug_Settings ModPlugWrapper::settings{};

ModPlugWrapper::ModPlugWrapper(string filename) : StandardWrapper(filename)
{
    this->initAttr();
}

ModPlugWrapper::ModPlugWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : StandardWrapper(filename, offset, len)
{
    this->initAttr();
}

void ModPlugWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::int32;
}

ModPlugWrapper::~ModPlugWrapper ()
{
    this->releaseBuffer();
    this->close();
}


void ModPlugWrapper::open ()
{
    // avoid multiple calls to open()
    if(this->infile!=nullptr)
    {
        return;
    }

    this->infile = fopen(this->Filename.c_str(), "rb");
    if (this->infile == nullptr)
    {
        THROW_RUNTIME_ERROR("fopen failed, errno: " << string(strerror(errno)));
    }

    int fd = fileno(this->infile);
    this->infilelen = getFileSize(fd);
    this->infilebuf = static_cast<unsigned char*>(mmap(nullptr, this->infilelen, PROT_READ, MAP_SHARED, fd, 0));
    if (this->infilebuf == MAP_FAILED)
    {
        THROW_RUNTIME_ERROR("mmap failed for File '" << this->Filename << "'");
    }
    
    ModPlug_GetSettings(&ModPlugWrapper::settings);
    ModPlugWrapper::settings.mFlags  = MODPLUG_ENABLE_OVERSAMPLING;
    
    if(Config::ModPlugEnableNoiseRed)
    {
        ModPlugWrapper::settings.mFlags |= MODPLUG_ENABLE_NOISE_REDUCTION;
    }
    if(Config::ModPlugEnableReverb)
    {
        ModPlugWrapper::settings.mFlags |= MODPLUG_ENABLE_REVERB;
    }
    if(Config::ModPlugEnableBass)
    {
        ModPlugWrapper::settings.mFlags |= MODPLUG_ENABLE_MEGABASS;
    }
    if(Config::ModPlugEnableSurround)
    {
        ModPlugWrapper::settings.mFlags |= MODPLUG_ENABLE_SURROUND;
    }
    
    ModPlugWrapper::settings.mChannels = this->Format.Channels = 2;
    ModPlugWrapper::settings.mBits = 32;
    ModPlugWrapper::settings.mFrequency = this->Format.SampleRate = Config::ModPlugSampleRate;
    
    ModPlugWrapper::settings.mResamplingMode = MODPLUG_RESAMPLE_FIR;

    ModPlugWrapper::settings.mStereoSeparation = 128;
    ModPlugWrapper::settings.mMaxMixChannels = 64;

    ModPlugWrapper::settings.mReverbDepth = Config::ModPlugReverbDepth;
    ModPlugWrapper::settings.mReverbDelay = Config::ModPlugReverbDelay;
    
    ModPlugWrapper::settings.mBassAmount = Config::ModPlugBassAmount;
    ModPlugWrapper::settings.mBassRange = Config::ModPlugBassRange;
    
    ModPlugWrapper::settings.mSurroundDepth = Config::ModPlugSurroundDepth;
    ModPlugWrapper::settings.mSurroundDelay = Config::ModPlugSurroundDelay;
    
    ModPlugWrapper::settings.mLoopCount = 2;
    
    // settings must be set before loading a file
    ModPlug_SetSettings(&ModPlugWrapper::settings);
    
    this->handle = ModPlug_Load(this->infilebuf, this->infilelen);
    if(this->handle == nullptr)
    {
        THROW_RUNTIME_ERROR("ModPlug_Load failed on file '" << this->Filename << "'");
    }
    
    this->fileLen = ModPlug_GetLength(this->handle);
}

void ModPlugWrapper::close() noexcept
{
    if(this->handle != nullptr)
    {
        ModPlug_Unload(this->handle);
        this->handle = nullptr;
    }
    
    if(this->infilebuf!=nullptr)
    {
        munmap(this->infilebuf, this->infilelen);
        this->infilebuf=nullptr;
        this->infilelen=0;
    }

    if(this->infile!=nullptr)
    {
        fclose(this->infile);
        this->infile=nullptr;
    }
}

void ModPlugWrapper::fillBuffer()
{
    StandardWrapper::fillBuffer(this);
}

void ModPlugWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    STANDARDWRAPPER_RENDER(int32_t,
                           int ret = ModPlug_Read(this->handle, pcm, framesToDoNow*this->Format.Channels*sizeof(int32_t));
                           framesToDoNow = ret / (this->Format.Channels * sizeof(int32_t));
                          )
}

frame_t ModPlugWrapper::getFrames () const
{
    return msToFrames(this->fileLen.Value, this->Format.SampleRate);
}
