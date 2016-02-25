#include <iostream>
#include <string>

#include "Playlist.h"
#include "PlaylistFactory.h"
#include "Player.h"

using namespace std;

int main()
{
 string filenames = "/home/tom/Eigene Programme/ANMP/src/bmenuhard.wav";
 
 
 IPlaylist* plist = new Playlist();
 
 PlaylistFactory::addSong(*plist, filenames);
 
 Player p(plist);

   
   p.init();
   p.play();
   while(true);
  
 
 return 0;
 
 
}
