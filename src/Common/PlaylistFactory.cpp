#include "PlaylistFactory.h"

// Constructors/Destructors
//  

PlaylistFactory::PlaylistFactory () {
}

PlaylistFactory::~PlaylistFactory () { }

//  
// Methods
//  


// Accessor methods
//  


// Other methods
//  


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
void PlaylistFactory::addSong (IPlaylist& playlist, string filePath, string offset = "")
{
string ext = getFileExtension(filePath);
PCMHolder* pcm=nullptr;
  if (ext=="cue")
  {
    // parse cue and call addSong()
    return true;
  }
  else if (iEquals(ext, "usf") || iEquals(ext, "miniusf"))
  {
    pcm = new LazyusfWrapper(filePath);
      try
      {
	pcm.open();
      }
      catch
      {
      // log and cancel
      }
  }
  else if(iEquals(ext, "opus"))
  {
    pcm = new OpusWrapper(filePath);
    try
    {
      pcm.open();
    }
    catch
    {
    // log and cancel
    }


  }
  else // so many formats to test here, try and error
  {
    // TODO: pass offset to libsnd
    pcm = new LibSNDWrapper(filePath);

    try
    {
      pcm.open();
    }
    catch
    {
      pcm.close();
      delete pcm;
      pcm=new VGMStreamWrapper(filePath);
      try
      {
	pcm.open();
      }
      catch
      {
	pcm.close();
	delete pcm;
	pcm=nullptr;
      }
    }

    if(pcm==nullptr)
    {
    // log it away
    // return false?
    }
  }

    core::tree loops = getLoopFromPCM(pcm);

Song s(pcm, loops);

playlist.add(s);
}


/**
 * @return core::tree
 * @param  p
 */
core::tree PlaylistFactory::getLoopFromPCM (PCMHolder* p)
{
  core::tree loopTree;
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
  return (i.end-i.start)>(j.end-j.start);
}


