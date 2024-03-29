#include "PlaylistFactory.h"

#include "AtomicWrite.h"
#include "CommonExceptions.h"
#include "Song.h"

#ifdef USE_LAZYUSF
#include "LazyusfWrapper.h"
#endif

#ifdef USE_AOPSF
#include "AopsfWrapper.h"
#endif

#ifdef USE_LIBSND
#include "LibSNDWrapper.h"
#endif

#ifdef USE_LIBMAD
#include "LibMadWrapper.h"
#endif

#ifdef USE_LIBGME
#include "LibGMEWrapper.h"
#endif

#ifdef USE_MODPLUG
#include "ModPlugWrapper.h"
#endif

#ifdef USE_OPENMPT
#include "OpenMPTWrapper.h"
#endif

#ifdef USE_VGMSTREAM
#include "VGMStreamWrapper.h"
#endif

#ifdef USE_FLUIDSYNTH
#include "N64CSeqWrapper.h"
#endif

#ifdef USE_FFMPEG
#include "FFMpegWrapper.h"
#endif

#ifdef USE_SMF
#include "MidiWrapper.h"
#endif

#include "Common.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <memory>

#ifdef USE_CUE
extern "C" {
#include <libcue/libcue.h>
}
#endif

using namespace std;

#ifdef USE_CUE

void PlaylistFactory::parseCue(std::vector<Song*> &playlist, const std::string &filePath)
{
#define cue_assert(ERRMSG, COND)                                                                                    \
    if (!(COND))                                                                                                    \
    {                                                                                                               \
        throw runtime_error(std::string("Error: Failed to parse CUE-Sheet file \"") + filePath + "\" (" + ERRMSG + ")"); \
    }

    std::unique_ptr<FILE, decltype(&fclose)> f(fopen(filePath.c_str(), "r"), &fclose);
    cue_assert("failed to open cue file real-only", f != nullptr);
    
    std::unique_ptr<Cd, decltype(&cd_delete)> cd(cue_parse_file(f.get()), &cd_delete);
    cue_assert("error parsing CUE", cd != nullptr);

    SongInfo overridingMetadata;
    {
        // get metadata of overall CD
        Cdtext *albumtext = cd_get_cdtext(cd.get());

        const char *val = cdtext_get(PTI_PERFORMER, albumtext);
        if (val != nullptr)
        {
            // overall CD interpret
            overridingMetadata.Artist = std::string(val);
        }

        val = cdtext_get(PTI_COMPOSER, albumtext);
        if (val != nullptr)
        {
            // overall CD Composer
            overridingMetadata.Composer = std::string(val);
        }

        val = cdtext_get(PTI_TITLE, albumtext);
        if (val != nullptr)
        {
            // album title
            overridingMetadata.Album = std::string(val);
        }
    }

    int ntrk = cd_get_ntrack(cd.get());
    for (int i = 0; i < ntrk; i++)
    {
        Track *track = cd_get_track(cd.get(), i + 1);
        cue_assert("error getting track", track != nullptr);

        const char *realAudioFile = track_get_filename(track);
        cue_assert("error getting track filename", realAudioFile != nullptr);

        Cdtext *cdtext = track_get_cdtext(track);
        cue_assert("error getting track CDTEXT", cdtext != nullptr);

        {
            stringstream ss;
            ss << setw(2) << setfill('0') << i + 1;
            overridingMetadata.Track = ss.str();

            const char *val = cdtext_get(PTI_PERFORMER, cdtext);
            if (val != nullptr)
            {
                overridingMetadata.Artist = std::string(val);
            }

            val = cdtext_get(PTI_COMPOSER, cdtext);
            if (val != nullptr)
            {
                overridingMetadata.Composer = std::string(val);
            }

            val = cdtext_get(PTI_TITLE, cdtext);
            if (val != nullptr)
            {
                overridingMetadata.Title = std::string(val);
            }

            val = cdtext_get(PTI_GENRE, cdtext);
            if (val != nullptr)
            {
                overridingMetadata.Genre = std::string(val);
            }
        }

        int strangeFramesStart = track_get_start(track);

        int strangeFramesLen = track_get_length(track);
        Nullable<size_t> len;
        if (strangeFramesLen == -1 || strangeFramesLen == 0)
        {
            len = nullptr;
        }
        else
        {
            len = strangeFramesLen;
            len.Value *= 1000;
            len.Value /= 75;
        }

        PlaylistFactory::addSong(playlist, mydirname(filePath).append("/").append(realAudioFile), strangeFramesStart * 1000 / 75, len, overridingMetadata);
    }
#undef cue_assert
}
#endif


bool PlaylistFactory::addSong(std::vector<Song*> &playlist, const std::string& filePath, Nullable<size_t> offset, Nullable<size_t> len, Nullable<SongInfo> overridingMetadata)
{
    Song *pcm = nullptr;

    std::string ext = getFileExtension(filePath);

    if (iEquals(ext, "ebur128") ||
        iEquals(ext, "mood") ||
        iEquals(ext, "usflib") ||
        iEquals(ext, "sf2") || // libmad forever busy
        iEquals(ext, "dls") ||
        iEquals(ext, "txt") || // libmad converts it to sound
        iEquals(ext, "bash") ||
        iEquals(ext, "zip") || // libsmf assertion fail
        iEquals(ext, "tar") ||
        iEquals(ext, "7z") ||
        iEquals(ext, "gz") ||
        iEquals(ext, "bz") ||
        iEquals(ext, "bz2") ||
        iEquals(ext, "bzip") ||
        iEquals(ext, "xz") ||
        iEquals(ext, "rar") ||
        iEquals(ext, "png") || // libmad assertion fail
        iEquals(ext, "jpg"))
    {
        // moodbar and loudness files, dont care
        return false;
    }
    else if (iEquals(ext, "cue"))
    {
#ifdef USE_CUE
        try
        {
            PlaylistFactory::parseCue(playlist, filePath);
            return true;
        }
        catch (const runtime_error &e)
        {
            CLOG(LogLevel_t::Error, "failed parsing '" << filePath << "' error: '" << e.what() << "'");
        }
#endif
    }

#ifdef USE_FLUIDSYNTH
    else if (iEquals(ext, "mid") || iEquals(ext, "midi"))
    {
        PlaylistFactory::tryWith<MidiWrapper>(pcm, filePath, offset, len);
    }
    else if (iEquals(ext, "cmf") || iEquals(ext, "btmf"))
    {
        PlaylistFactory::tryWith<N64CSeqWrapper>(pcm, filePath, offset, len);
    }
#endif

#ifdef USE_LAZYUSF
    else if (iEquals(ext, "usf") || iEquals(ext, "miniusf"))
    {
        PlaylistFactory::tryWith<LazyusfWrapper>(pcm, filePath, offset, len);
    }
#endif

#ifdef USE_AOPSF
    else if (iEquals(ext, "psf") || iEquals(ext, "minipsf") || iEquals(ext, "psf2") || iEquals(ext, "minipsf2"))
    {
        PlaylistFactory::tryWith<AopsfWrapper>(pcm, filePath, offset, len);
    }
#endif

#if defined(USE_MODPLUG) || defined(USE_OPENMPT)
    else if (iEquals(ext, "MOD") || iEquals(ext, "MDZ") || iEquals(ext, "MDR") || iEquals(ext, "MDGZ") ||
             iEquals(ext, "S3M") || iEquals(ext, "S3Z") || iEquals(ext, "S3R") || iEquals(ext, "S3GZ") ||
             iEquals(ext, "XM") || iEquals(ext, "XMZ") || iEquals(ext, "XMR") || iEquals(ext, "XMGZ") ||
             iEquals(ext, "IT") || iEquals(ext, "ITZ") || iEquals(ext, "ITR") || iEquals(ext, "ITGZ") ||
             iEquals(ext, "669") || iEquals(ext, "AMF") || iEquals(ext, "AMS") || iEquals(ext, "DBM") || iEquals(ext, "DMF") || iEquals(ext, "DSM") || iEquals(ext, "FAR") || iEquals(ext, "MDL") || iEquals(ext, "MED") || iEquals(ext, "MTM") || iEquals(ext, "OKT") || iEquals(ext, "PTM") || iEquals(ext, "STM") || iEquals(ext, "ULT") || iEquals(ext, "UMX") || iEquals(ext, "MT2") || iEquals(ext, "PSM"))
    {
#ifdef USE_OPENMPT
        PlaylistFactory::tryWith<OpenMPTWrapper>(pcm, filePath, offset, len);
#endif
#ifdef USE_MODPLUG
        PlaylistFactory::tryWith<ModPlugWrapper>(pcm, filePath, offset, len);
#endif
    }
#endif

#ifdef USE_LIBGME
    else if ((iEquals(ext, "gbs") || iEquals(ext, "nsf") || iEquals(ext, "kss")) // for files that can contain multiple sub-songs
             && !offset.hasValue && !len.hasValue) // and this is the first call for this file, i.e. no sub-songs and song lengths have been specified
    {
        // ... try to parse that file

        Music_Emu *emu = nullptr;
        gme_err_t msg = gme_open_file(filePath.c_str(), &emu, gme_info_only);
        if (msg || emu == nullptr)
        {
            if (emu != nullptr)
            {
                gme_delete(emu);
            }

            if (msg)
            {
                CLOG(LogLevel_t::Error, "though assumed GME compatible file, got error: '" << msg << "' for file '" << filePath << "'");
            }
            else
            {
                CLOG(LogLevel_t::Error, "though assumed GME compatible file, GME failed without error for file '" << filePath << "'");
            }

            return false;
        }

        int trackCount = gme_track_count(emu);

        gme_delete(emu);

        for (int i = 0; i < trackCount; i++)
        {
            // add each sub-song again, with default playing time of 3 mins
            PlaylistFactory::addSong(playlist, filePath, i, 3 * 60 * 1000);
        }
    }
#endif

#ifdef USE_LIBMAD
    else if (iEquals(ext, "mp3") || iEquals(ext, "mp2"))
    {
        goto l_LIBMAD;
    }
#endif
    else
    {
// so many formats to test here, try and error
// note the order of the librarys to test
// we start with libraries that only read well defined audiofiles, i.e. where every header and every single bit is set as the library expects it, so the resulting audible output sounds absolutly perfect
// and we end up in testing libraries which also eat up every garbage of binary streams, resulting in some ugly crack noises

#ifdef USE_LIBSND
        // most common file types (WAVE, FLAC, Sun / NeXT AU, OGG VORBIS, AIFF, etc.)
        PlaylistFactory::tryWith<LibSNDWrapper>(pcm, filePath, offset, len);
#endif

#ifdef USE_LIBGME
        // emulated sound formats from old video consoles (SuperFamicon, Famicon, GAMEBOY, etc.)
        PlaylistFactory::tryWith<LibGMEWrapper>(pcm, filePath, offset, len);
#endif

#ifdef USE_VGMSTREAM
        {
            size_t len;
            const char **extList = vgmstream_get_formats(&len);

            for (size_t i = 0; i < len && pcm == nullptr; i++)
            {
                if (iEquals(ext, extList[i]))
                {
                    // most fileformats from videogames
                    // also eats raw pcm files (although they'll may have wrong samplerate
                    PlaylistFactory::tryWith<VGMStreamWrapper>(pcm, filePath, offset, len);
                }
            }
        }
#endif

#ifdef USE_FFMPEG
        // OPUS, videofiles, etc.
        PlaylistFactory::tryWith<FFMpegWrapper>(pcm, filePath, offset, len);
#endif

// !!! libmad always has to be last !!!
// libmad eats up every garbage of binary (= non MPEG audio shit)
// thus always make sure libmad is the very last try to read any audio file
#ifdef USE_LIBMAD
    // mp3 exclusive
    l_LIBMAD:
        PlaylistFactory::tryWith<LibMadWrapper>(pcm, filePath, offset, len);
#endif
    }

    if (pcm == nullptr)
    {
        CLOG(LogLevel_t::Error, "No library seems to support that file: \"" << filePath << "\"");
        return false;
    }

    pcm->buildLoopTree();
    pcm->buildMetadata();

    // correct metadata if possible
    if (overridingMetadata.hasValue)
    {
        if (overridingMetadata.Value.Title != "")
        {
            pcm->Metadata.Title = overridingMetadata.Value.Title;
        }
        if (overridingMetadata.Value.Track != "")
        {
            pcm->Metadata.Track = overridingMetadata.Value.Track;
        }
        if (overridingMetadata.Value.Artist != "")
        {
            pcm->Metadata.Artist = overridingMetadata.Value.Artist;
        }
        if (overridingMetadata.Value.Album != "")
        {
            pcm->Metadata.Album = overridingMetadata.Value.Album;
        }
        if (overridingMetadata.Value.Composer != "")
        {
            pcm->Metadata.Composer = overridingMetadata.Value.Composer;
        }
        if (overridingMetadata.Value.Year != "")
        {
            pcm->Metadata.Year = overridingMetadata.Value.Year;
        }
        if (overridingMetadata.Value.Genre != "")
        {
            pcm->Metadata.Genre = overridingMetadata.Value.Genre;
        }
        if (overridingMetadata.Value.Comment != "")
        {
            pcm->Metadata.Comment = overridingMetadata.Value.Comment;
        }
    }

    pcm->close();

    playlist.push_back(pcm);

    return true;
}

template<typename T>
void PlaylistFactory::tryWith(Song *(&pcm), const std::string &filePath, Nullable<size_t> offset, Nullable<size_t> len)
{
    if (pcm == nullptr)
    {
        try
        {
            pcm = new T(filePath, offset, len);
            pcm->open();

            if (pcm->getFrames() <= 0)
            {
                THROW_RUNTIME_ERROR("Nothing to play, refusing to add file: '" << filePath << "'");
            }
        }
        catch (const exception &e)
        {
            CLOG(LogLevel_t::Error, e.what());
            delete pcm;
            pcm = nullptr;
        }
    }
}
