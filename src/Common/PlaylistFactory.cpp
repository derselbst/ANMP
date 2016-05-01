#include "PlaylistFactory.h"

#include "IPlaylist.h"
#include "Song.h"

#ifdef USE_LAZYUSF
#include "LazyusfWrapper.h"
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

#ifdef USE_VGMSTREAM
#include "VGMStreamWrapper.h"
#endif

#ifdef USE_FFMPEG
#include "FFMpegWrapper.h"
#endif

#include "Common.h"

#include <sstream>
#include <iomanip>
#include <cstring>
#include <iostream>
#include <libgen.h>
#include <linux/limits.h>

#ifdef USE_CUE
extern "C"
{
#include <libcue/libcue.h>
}
#endif



#ifdef USE_CUE
/**
 * @param  playlist
 * @param  filePath
 * @param  offset
 */

void PlaylistFactory::parseCue(IPlaylist& playlist, const string& filePath)
{
#define cue_assert(ERRMSG, COND) \
  if(!(COND))\
  {\
     throw runtime_error(string("Error: Failed to parse CUE-Sheet file \"") + filePath + "\" ("+ERRMSG+")");\
  }

    FILE *f = fopen(filePath.c_str(), "r");
    Cd *cd = cue_parse_file(f);
    cue_assert ("error parsing CUE", cd != NULL);

    int ntrk = cd_get_ntrack (cd);
    char temp[PATH_MAX];
    for(int i=0; i< ntrk; i++)
    {
        Track* track = cd_get_track (cd, i+1);
        cue_assert ("error getting track", track != NULL);

        char* realAudioFile = track_get_filename (track);
        cue_assert ("error getting track filename", realAudioFile != NULL);

      Cdtext* cdtext = track_get_cdtext (track);
      cue_assert ("error getting track CDTEXT", cdtext != NULL);

      SongInfo overridingMetadata;
      {
	  stringstream ss;
	  ss << setw(2) << setfill('0') << i+1;
	  overridingMetadata.Track = ss.str();
	
	  char* val = cdtext_get (PTI_PERFORMER, cdtext);
	  if(val != NULL)
	  {
	    overridingMetadata.Artist = string(val);
	  }
	  
	  val = cdtext_get (PTI_COMPOSER, cdtext);
	  if(val != NULL)
	  {
	    overridingMetadata.Composer = string(val);
	  }

	  val = cdtext_get (PTI_TITLE, cdtext);
	  if(val != NULL)
	  {
	    overridingMetadata.Title = string(val);
	  }
	  
	  val = cdtext_get (PTI_GENRE, cdtext);
	  if(val != NULL)
	  {
	    overridingMetadata.Genre = string(val);
	  }
      }
      
        int strangeFramesStart = track_get_start (track);

        int strangeFramesLen = track_get_length (track);
	Nullable<size_t> len;
        if(strangeFramesLen == -1 || strangeFramesLen == 0)
        {
            len = nullptr;
        }
        else
	{
	  len = strangeFramesLen;
	  len.Value *= 1000;
	  len.Value /= 75;
	}


        strcpy(temp, filePath.c_str());

        PlaylistFactory::addSong(playlist, string(dirname(temp)).append("/").append(realAudioFile), strangeFramesStart*1000/75, len, overridingMetadata);
    }
    cd_delete (cd);
#undef cue_assert
}
#endif

#define TRY_WITH(LIBWRAPPER)\
if(pcm==nullptr)\
{\
    pcm = new LIBWRAPPER(filePath, offset, len);\
    try\
    {\
	pcm->open();\
    }\
    catch(exception& e)\
    {\
	cerr << e.what() << endl;\
	pcm->close();\
	delete pcm;\
	pcm=nullptr;\
    }\
}

Song* PlaylistFactory::addSong (IPlaylist& playlist, const string filePath, Nullable<size_t> offset, Nullable<size_t> len, Nullable<SongInfo> overridingMetadata)
{
    string ext = getFileExtension(filePath);
    Song* pcm=nullptr;

    if (iEquals(ext,"cue"))
    {
#ifdef USE_CUE
        PlaylistFactory::parseCue(playlist, filePath);
#endif
    }
#ifdef USE_LAZYUSF
    else if (iEquals(ext, "usf") || iEquals(ext, "miniusf"))
    {
        TRY_WITH(LazyusfWrapper)
    }
#endif
#ifdef USE_LIBGME
    else if((iEquals(ext, "gbs") || iEquals(ext, "nsf")) && !offset.hasValue && !len.hasValue)
    {
        Music_Emu * emu=nullptr;
        gme_err_t msg = gme_open_file(filePath.c_str(), &emu, gme_info_only);
	if(msg || emu == nullptr)
	{
	    return pcm;
	}

	int trackCount = gme_track_count(emu);
      
        gme_delete(emu);
	
	for(int i=0; i<trackCount; i++)
	{
	  PlaylistFactory::addSong(playlist, filePath, i, 3*60*1000);
	}
    }
#endif
#ifdef USE_LIBMAD
    else if(iEquals(ext, "mp3"))
    {
        goto l_LIBMAD;
    }
#endif
    else
    { // so many formats to test here, try and error
      // note the order of the librarys to test
      // we start with libraries that only read well defined audiofiles, i.e. where every header and every single bit is set as the library expects it, so the resulting audible output sounds absolutly perfect
      // and we end up in testing libraries which also eat up every garbage of binary streams, resulting in some ugly crack noises
        
#ifdef USE_LIBSND
      // most common file types (WAVE, FLAC, Sun / NeXT AU, OGG VORBIS, AIFF, etc.)
      TRY_WITH(LibSNDWrapper)
#endif

#ifdef USE_LIBGME
      // emulated sound formats from old video consoles (SuperFamicon, Famicon, GAMEBOY, etc.)
      TRY_WITH(LibGMEWrapper)
#endif
      
#ifdef USE_FFMPEG
      // OPUS, videofiles, etc.
      TRY_WITH(FFMpegWrapper)
#endif
      
#ifdef USE_VGMSTREAM
      // most fileformats from videogames
      // also eats raw pcm files (although they'll may have wrong samplerate
      TRY_WITH(VGMStreamWrapper)
#endif

// !!! libmad always has to be last !!!
// libmad eats up every garbage of binary (= non MPEG audio shit)
// thus always make sure libmad is the very last try to read any audio file
#ifdef USE_LIBMAD
      // mp3 exclusive
      l_LIBMAD:
      TRY_WITH(LibMadWrapper)
#endif
    }

    if(pcm==nullptr)
    {
        // log it away
        return pcm;
    }

    pcm->buildLoopTree();
    pcm->buildMetadata();
    
    // correct metadata if possible
    if(overridingMetadata.hasValue)
    {
      if(overridingMetadata.Value.Title != "")
      {
	pcm->Metadata.Title = overridingMetadata.Value.Title;
      }
      if(overridingMetadata.Value.Track != "")
      {
	pcm->Metadata.Track = overridingMetadata.Value.Track;
      }
      if(overridingMetadata.Value.Artist != "")
      {
	pcm->Metadata.Artist = overridingMetadata.Value.Artist;
      }
      if(overridingMetadata.Value.Album != "")
      {
	pcm->Metadata.Album = overridingMetadata.Value.Album;
      }
      if(overridingMetadata.Value.Composer != "")
      {
	pcm->Metadata.Composer = overridingMetadata.Value.Composer;
      }
      if(overridingMetadata.Value.Year != "")
      {
	pcm->Metadata.Year = overridingMetadata.Value.Year;
      }
      if(overridingMetadata.Value.Genre != "")
      {
	pcm->Metadata.Genre = overridingMetadata.Value.Genre;
      }
      if(overridingMetadata.Value.Comment != "")
      {
	pcm->Metadata.Comment = overridingMetadata.Value.Comment;
      }
    }

    pcm->close();

    playlist.add(pcm);

    return pcm;
}



