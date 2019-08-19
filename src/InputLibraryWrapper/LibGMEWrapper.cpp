#include "LibGMEWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Config.h"

#include <iomanip>
#include <utility>

LibGMEWrapper::LibGMEWrapper(string filename)
: StandardWrapper(std::move(filename))
{
    this->initAttr();
}

// NOTE! for this class we use Song::fileOffset as track offset (i.e. track num) for libgme
LibGMEWrapper::LibGMEWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len)
: StandardWrapper(std::move(filename), offset, len)
{
    this->initAttr();
}

void LibGMEWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::int16;
}

LibGMEWrapper::~LibGMEWrapper()
{
    this->close();
    this->releaseBuffer();
}

/* Print any warning for most recent emulator action (load, start_track, play) */
void LibGMEWrapper::printWarning(Music_Emu *emu)
{
    const char *warning = gme_warning(emu);
    if (warning)
    {
        CLOG(LogLevel_t::Warning, warning);
    }
}

void LibGMEWrapper::open()
{
    if (this->handle != nullptr)
    {
        return;
    }

    gme_type_t emuType;
    gme_err_t msg = gme_identify_file(this->Filename.c_str(), &emuType);

    if (msg)
    {
        THROW_RUNTIME_ERROR("libgme failed on file \"" << this->Filename << ")\""
                                                       << " with message " << msg);
    }

#if GME_VERSION > 0x000601
    if (gConfig.gmeMultiChannel)
    {
        this->handle = gme_new_emu_multi_channel(emuType, gConfig.gmeSampleRate);
    }
    else
#else
#warning "libgme is too old to support multichannel rendering, falling back to stereo."
#endif
    {
        this->handle = gme_new_emu(emuType, gConfig.gmeSampleRate);
    }

    if (this->handle == nullptr)
    {
        THROW_RUNTIME_ERROR("failed to create MusicEmu, out of memory?!");
    }

    msg = gme_load_file(this->handle, this->Filename.c_str());
    if (msg)
    {
        THROW_RUNTIME_ERROR("libgme's second attempt failed on file \"" << this->Filename << ")\""
                                                                        << " with message " << msg);
    }

    bool multiChannelSupport = false;
#if GME_VERSION > 0x000601
    multiChannelSupport = gme_multi_channel(this->handle);
    if (multiChannelSupport)
    {
        this->Format.SetVoices(8);
        // each of the eight voices will be rendered to a stereo channel
        for (unsigned int i = 0; i < this->Format.VoiceChannels.size(); i++)
        {
            this->Format.VoiceChannels[i] = 2;
        }

        // set voice names
        int voices = min(gme_voice_count(this->handle), 8);
        for (int i = 0; i < voices; i++)
        {
            this->Format.VoiceName[i] = gme_voice_name(this->handle, i);
        }
    }
    else
#endif
    {

        // there will always be one stereo channel, if this will be real stereo sound or only mono depends on the game
        this->Format.SetVoices(1);
        this->Format.VoiceChannels[0] = 2;
    }

    if (gConfig.gmeMultiChannel && !multiChannelSupport)
    {
        CLOG(LogLevel_t::Warning, "though requested, gme does not support multichannel rendering for file '" << this->Filename << "'");
    }

    if (multiChannelSupport)
    {
        CLOG(LogLevel_t::Info, "multichannel rendering activated for file '" << this->Filename << "'");
    }

    //     gme_mute_voices(this->handle, 0);


    LibGMEWrapper::printWarning(this->handle);

#if GME_VERSION >= 0x000600
    /* Enable most accurate sound emulation */
    gme_enable_accuracy(this->handle, gConfig.gmeAccurateEmulation);
#endif

    /* Add some stereo enhancement */
    gme_set_stereo_depth(this->handle, gConfig.gmeEchoDepth);

    /* Start track and begin fade at 10 seconds */
    int offset = this->fileOffset.hasValue ? this->fileOffset.Value : 0;
    msg = gme_start_track(this->handle, offset);
    if (msg)
    {
        THROW_RUNTIME_ERROR("libgme failed to set track no. " << offset << " for file \"" << this->Filename << "\" with message: " << msg);
    }

    this->printWarning(this->handle);

    msg = gme_track_info(this->handle, &this->info, offset);
    if (msg || this->info == nullptr)
    {
        THROW_RUNTIME_ERROR("libgme failed to retrieve track info for track no. " << offset << " for file \"" << this->Filename << "\" with message: " << msg);
    }

    auto oldLen = this->fileLen;
    if (gConfig.gmePlayForever)
    {
        this->fileLen = -1;
    }
    else
    {
        // if the file has no default duration
        if (this->info->length == -1)
        {
            // the total length is not specified, try to figure it out
            if(this->info->intro_length != -1 && this->info->loop_length != -1)
            {
                this->fileLen = this->info->intro_length + this->info->loop_length;
            }
            else
            {
                // use what GME thinks is best
                this->fileLen = this->info->play_length;
            }
        }
        else
        {
            // use the duration from file
            this->fileLen = this->info->length;
        }
    }

    // rebuild loop tree, to get proper infinite playback, if it was just enabled
    if (oldLen.hasValue || this->fileLen.Value != oldLen.Value || this->Format.SampleRate != gConfig.gmeSampleRate)
    {
        // the sample rate may have changed, if requested by user
        this->Format.SampleRate = gConfig.gmeSampleRate;
        // so we have to build up the loop tree again
        this->buildLoopTree();
    }

    gme_set_fade(this->handle, this->fileLen.Value);
}

void LibGMEWrapper::close() noexcept
{
    if (this->handle != nullptr)
    {
        gme_delete(this->handle);
        this->handle = nullptr;

        gme_free_info(info);
        this->info = nullptr;
    }
}

void LibGMEWrapper::fillBuffer()
{
    StandardWrapper::fillBuffer(this);
}

void LibGMEWrapper::render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender)
{
STANDARDWRAPPER_RENDER(int16_t, gme_play(this->handle, framesToDoNow * Channels, pcm))
}

frame_t LibGMEWrapper::getFrames() const
{
    return msToFrames(this->fileLen.Value, this->Format.SampleRate);
}

vector<loop_t> LibGMEWrapper::getLoopArray() const noexcept
{
    vector<loop_t> res;

    if (this->info != nullptr && this->wholeSong() && this->info->intro_length != -1 && this->info->loop_length != -1)
    {
        loop_t l;
        l.start = msToFrames(this->info->intro_length, this->Format.SampleRate);
        l.stop = msToFrames(this->info->intro_length + this->info->loop_length, this->Format.SampleRate);
        l.count = 0;
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

    if (this->fileOffset.hasValue)
    {
        stringstream ss;
        ss << setw(2) << setfill('0') << this->fileOffset.Value;
        this->Metadata.Track = ss.str();
    }
}

// true if we can hold the whole song in memory
bool LibGMEWrapper::wholeSong() const
{
    return this->fileLen.Value != -1;
}
