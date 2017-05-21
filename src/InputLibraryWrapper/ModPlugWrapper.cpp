#include "ModPlugWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"

#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

ModPlug_Settings ModPlugWrapper::settings;

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
    
    if(gConfig.ModPlugEnableNoiseRed)
    {
        ModPlugWrapper::settings.mFlags |= MODPLUG_ENABLE_NOISE_REDUCTION;
    }
    if(gConfig.ModPlugEnableReverb)
    {
        ModPlugWrapper::settings.mFlags |= MODPLUG_ENABLE_REVERB;
    }
    if(gConfig.ModPlugEnableBass)
    {
        ModPlugWrapper::settings.mFlags |= MODPLUG_ENABLE_MEGABASS;
    }
    if(gConfig.ModPlugEnableSurround)
    {
        ModPlugWrapper::settings.mFlags |= MODPLUG_ENABLE_SURROUND;
    }
    
    this->Format.SetVoices(1);
    ModPlugWrapper::settings.mChannels = this->Format.VoiceChannels[0] = 2;
    ModPlugWrapper::settings.mBits = 32;
    ModPlugWrapper::settings.mFrequency = this->Format.SampleRate = gConfig.ModPlugSampleRate;
    
    ModPlugWrapper::settings.mResamplingMode = MODPLUG_RESAMPLE_FIR;

    ModPlugWrapper::settings.mStereoSeparation = 128;
    ModPlugWrapper::settings.mMaxMixChannels = 64;

    ModPlugWrapper::settings.mReverbDepth = gConfig.ModPlugReverbDepth;
    ModPlugWrapper::settings.mReverbDelay = gConfig.ModPlugReverbDelay;
    
    ModPlugWrapper::settings.mBassAmount = gConfig.ModPlugBassAmount;
    ModPlugWrapper::settings.mBassRange = gConfig.ModPlugBassRange;
    
    ModPlugWrapper::settings.mSurroundDepth = gConfig.ModPlugSurroundDepth;
    ModPlugWrapper::settings.mSurroundDelay = gConfig.ModPlugSurroundDelay;
    
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
                           int ret = ModPlug_Read(this->handle, pcm, framesToDoNow*Channels*sizeof(int32_t));
                           framesToDoNow = ret / (Channels * sizeof(int32_t));
                          )
    
    this->doAudioNormalization(static_cast<int32_t*>(bufferToFill), framesToRender);
}

frame_t ModPlugWrapper::getFrames () const
{
    return msToFrames(this->fileLen.Value, this->Format.SampleRate);
}
