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
void PlaylistFactory::addSongs (IPlaylist playlist, vector& filePaths)
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
void PlaylistFactory::addSong (IPlaylist playlist, string filePath, string offset = "")
{
string ext = getFileExtension(filePath);
PCMHolder* pcm;
if (ext=="cue")
{
   // parse cue and call addSong()
return true;
}
else if (iEquals(ext, "usf") || iEquals(ext,"miniusf"))
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

core::tree loops = getLoopFromPCM(pcm);

}
else // so many formats to test here, try and error
{
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



core::tree loops = getLoopFromPCM(pcm);
}


Song s(PCMholder, loops);

playlist.add(s);
}


/**
 * @return core::tree
 * @param  p
 */
core::tree PlaylistFactory::getLoopsFromPCM (PCMHolder* p)
{
core::tree loopTree;
loopTree.insert(0, p.frames, 1);


vector<loops> l = p.getLoops();
l.sort();
foreach loop in l
{
core::tree subNode = findRootLoopNode(loopTree, loop);
subNode.insert();
}
}


