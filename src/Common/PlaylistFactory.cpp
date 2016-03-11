#include <iostream>

#include "PlaylistFactory.h"
#include "Song.h"

#include "Common.h"
#include "CommonExceptions.h"
#include "types.h"

#include "LazyusfWrapper.h"
#include "LibSNDWrapper.h"
#include "OpusWrapper.h"
#include "VGMStreamWrapper.h"


/**
 * @param  playlist
 * @param  filePath
 * @param  offset
 */

PlaylistFactory::parseCue(string& filepath)
{
#define cue_assert(ERRMSG, COND) \
  if(!(COND))\
  {\
     throw runtime_error(string("Error: Failed to parse CUE-Sheet file \"") + filepath + "\" ("+ERRMSG+")");\
  }
  
  FILE *f = fopen(filepath.c_str());
  Cd *cd = cue_parse_file(f);
  cue_assert ("error parsing CUE", cd != NULL);
  
  int ntrk = cd_get_ntrack (cd);
   for(int i=0; i< ntrk; i++)
   {
      Track* track = cd_get_track (cd, 1);
      cue_assert ("error getting track", track != NULL);

      val = track_get_filename (track);
      cue_assert ("error getting track filename", val != NULL);

      cdtext = track_get_cdtext (track);
      cue_assert ("error getting track CDTEXT", cdtext != NULL);

      val = cdtext_get (PTI_PERFORMER, cdtext);
      cue_assert ("error getting track performer", val != NULL);

      val = cdtext_get (PTI_TITLE, cdtext);
      cue_assert ("error getting track title", val != NULL);

      int strangeFramesStart = track_get_start (track);
      strangeFramesStart*1000/75
      
      int strangeFramesLen = track_get_length (track);
   }
  cd_delete (cd);
#undefine cue_assert
}

Song* PlaylistFactory::addSong (IPlaylist& playlist, string filePath, string offset)
{
    string ext = getFileExtension(filePath);
    Song* pcm=nullptr;

    if (iEquals(ext,"cue"))
    {
        // parse cue and call addSong()
        return pcm;
    }
    else if (iEquals(ext, "usf") || iEquals(ext, "miniusf"))
    {
	pcm = new LazyusfWrapper(filePath);
	try
	{
	  pcm->open();
	}
	catch(exception& e)
	{
	      cerr << "Could not open " << filePath << "\nwhat(): " << e.what() << endl;
	      pcm->close();
	      delete pcm;
	      pcm=nullptr;
	}
    }
    else if(iEquals(ext, "opus"))
    {
//     TODO: implement me
//     pcm = new OpusWrapper(filePath);
//     try
//     {
//       pcm.open();
//     }
//     catch
//     {
//     // log and cancel
//     }
        return pcm;

    }
    else // so many formats to test here, try and error
    {
        // TODO: pass offset to libsnd
        pcm = new LibSNDWrapper(filePath);

        try
        {
            pcm->open();
        }
        //TODO catch correct exception
        catch(exception& e)
        {
            cerr << e.what() << endl;
            pcm->close();
            delete pcm;
//       TODO: implement me
//       pcm=new VGMStreamWrapper(filePath);
//       try
//       {
// 	pcm.open();
//       }
//       catch
//       {
// 	pcm.close();
// 	delete pcm;
            pcm=nullptr;
//       }
        }
    }

    if(pcm==nullptr)
    {
        // log it away
        return pcm;
    }

    pcm->buildLoopTree();

    pcm->close();

    playlist.add(pcm);
    
    return pcm;
}



