#include <vector>
#include <iostream>

#include "PlaylistFactory.h"

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
PCMHolder* pcm=nullptr;

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

    core::tree<loop_t> loops = getLoopFromPCM(pcm);

Song* s = new Song(pcm, loops);

playlist.add(s);
}


/**
 * @return core::tree
 * @param  p
 */
core::tree<loop_t> PlaylistFactory::getLoopFromPCM (PCMHolder* p)
{
  core::tree<loop_t> loopTree;
  loop_t l;
  l.start = 0;
  l.stop = p->getFrames();
  l.count = 1;
  *loopTree = l;
/* TODO:implement!!
  vector<loop_t> l = p.getLoops();

  // TODO: make sure loop array is sorted correctly
  std::sort (l.begin(), l.end(), myLoopSort);
  foreach loop in l
  {
    core::tree subNode = findRootLoopNode(loopTree, loop);
    subNode.insert();
  }*/
return loopTree;
}

// should sort descendingly
bool myLoopSort(loop_t i,loop_t j)
{
  return (i.stop-i.start)>(j.stop-j.start);
}


