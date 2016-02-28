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
 * @param  filePaths
 */
void PlaylistFactory::addSongs (IPlaylist& playlist, vector<string>& filePaths)
{
    for (vector<string>::iterator it = filePaths.begin() ; it != filePaths.end(); ++it)
    {
        addSong(playlist, *it);
    }
}


/**
 * @param  playlist
 * @param  filePath
 * @param  offset
 */
void PlaylistFactory::addSong (IPlaylist& playlist, string filePath, string offset)
{
    string ext = getFileExtension(filePath);
    Song* pcm=nullptr;

    if (iEquals(ext,"cue"))
    {
        // parse cue and call addSong()
        return;
    }
    else if (iEquals(ext, "usf") || iEquals(ext, "miniusf"))
    {
        // TODO: implement me
//     pcm = new LazyusfWrapper(filePath);
//       try
//       {
// 	pcm.open();
//       }
//       catch
//       {
//       // log and cancel
//       }
        return;
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
        return;

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
        catch(runtime_error& e)
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
        return;
    }

    pcm->buildLoopTree();


    playlist.add(pcm);
}



