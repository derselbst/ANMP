#include "LibGMEWrapper.h"

#include "CommonExceptions.h"
#include "Config.h"
#include "Common.h"
#include "AtomicWrite.h"

#include <iomanip>

LibGMEWrapper::LibGMEWrapper(string filename) : StandardWrapper(filename)
{
    this->initAttr();
}

// NOTE! for this class we use Song::fileOffset as track offset (i.e. track num) for libgme
LibGMEWrapper::LibGMEWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : StandardWrapper(filename, offset, len)
{
    this->initAttr();
}

void LibGMEWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::int16;
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
        CLOG(LogLevel_t::Warning, warning);
    }
}

void LibGMEWrapper::open()
{
    if(this->handle!=nullptr)
    {
        return;
    }

#if GME_VERSION > 0x000601
    gme_err_t msg = gme_open_file(this->Filename.c_str(), &this->handle, gConfig.gmeSampleRate, gConfig.gmeMultiChannel);
#else
    gme_err_t msg = gme_open_file(this->Filename.c_str(), &this->handle, gConfig.gmeSampleRate);
#endif

    if(msg)
    {
        THROW_RUNTIME_ERROR("libgme failed on file \"" << this->Filename << ")\"" << " with message " << msg);
    }

    this->Format.Channels = 2;
#if GME_VERSION > 0x000601
    bool multiChannelSupport = gme_multi_channel(this->handle);
    
    // there will always be at least a stereo channel, if this will be real stereo sound or only mono depends on the game
    if(multiChannelSupport)
    {
        this->Format.Channels = 2*8;
    }
    
    if(gConfig.gmeMultiChannel && !multiChannelSupport)
    {
        CLOG(LogLevel_t::Warning, "though requested, gme does not support multichannel rendering for file '" << this->Filename << "'");
    }
    if(multiChannelSupport)
    {
        CLOG(LogLevel_t::Info, "multichannel rendering activated for file '" << this->Filename << "'");
    }
#endif

//     gme_mute_voices(this->handle, 0);

    if(this->handle == nullptr)
    {
        THROW_RUNTIME_ERROR("THIS SHOULD NEVER HAPPEN! libgme handle is NULL although no error was reported");
    }

    LibGMEWrapper::printWarning(this->handle);

#if GME_VERSION >= 0x000600
    /* Enable most accurate sound emulation */
    gme_enable_accuracy( this->handle, gConfig.gmeAccurateEmulation );
#endif

    /* Add some stereo enhancement */
    gme_set_stereo_depth( this->handle, gConfig.gmeEchoDepth );

    /* Start track and begin fade at 10 seconds */
    int offset = this->fileOffset.hasValue ? this->fileOffset.Value : 0;
    msg = gme_start_track( this->handle, offset );
    if(msg)
    {
        THROW_RUNTIME_ERROR("libgme failed to set track no. " << offset << " for file \"" << this->Filename << "\" with message: " << msg);
    }

    this->printWarning( this->handle );

    msg = gme_track_info( this->handle, &this->info, offset );
    if(msg || this->info == nullptr)
    {
        THROW_RUNTIME_ERROR("libgme failed to retrieve track info for track no. " << offset << " for file \"" << this->Filename << "\" with message: " << msg);
    }

    if(gConfig.gmePlayForever)
    {
        gme_set_fade(this->handle, -1);
    }
    else
    {
        if(!this->fileLen.hasValue)
        {
            // if we have no playing duration
            if(this->info->length==-1)
            {
                // ... and the file has no default duration
                // use 3 minutes as default
                this->fileLen.Value = 3*60*1000;
            }
            else
            {
                // use the duration from file
                this->fileLen.Value = this->info->length;
            }
        }
        gme_set_fade(this->handle, this->fileLen.Value);
    }

    if(this->Format.SampleRate != gConfig.gmeSampleRate)
    {
        // the sample rate may have changed, if requested by user
        this->Format.SampleRate = gConfig.gmeSampleRate;
        // so we have to build up the loop tree again
        this->buildLoopTree();
    }
}

void LibGMEWrapper::close() noexcept
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
    StandardWrapper::fillBuffer(this);
}

void LibGMEWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    STANDARDWRAPPER_RENDER(int16_t, gme_play(this->handle, framesToDoNow * this->Format.Channels, pcm))
}

frame_t LibGMEWrapper::getFrames () const
{
    return gConfig.gmePlayForever ? msToFrames(-1, this->Format.SampleRate) : msToFrames(this->fileLen.Value, this->Format.SampleRate);
}

vector<loop_t> LibGMEWrapper::getLoopArray () const noexcept
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

void LibGMEWrapper::buildMetadata() noexcept
{
    this->Metadata.Title = string(this->info->song);
    this->Metadata.Artist = this->Metadata.Composer = string(this->info->author);
    this->Metadata.Album = string(this->info->game);
    this->Metadata.Year = string(this->info->copyright);
    this->Metadata.Genre = "Videogame";
    this->Metadata.Comment = string(this->info->comment);

    if(this->fileOffset.hasValue)
    {
        stringstream ss;
        ss << setw(2) << setfill('0') << this->fileOffset.Value;
        this->Metadata.Track = ss.str();
    }
}

// true if we can hold the whole song in memory
bool LibGMEWrapper::wholeSong() const
{
    return !gConfig.gmePlayForever;
}
