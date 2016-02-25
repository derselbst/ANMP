#include <iostream>
#include <string>

#include "Playlist.h"
#include "PlaylistFactory.h"
#include "Player.h"

using namespace std;

int main()
{
 string filenames = "/path/to/some/wav.wav";
 
 
 IPlaylist* plist = new Playlist();
 
 PlaylistFactory::addSong(*plist, filenames);
 
 Player p(plist);
 /* TO BE TESTED:
  * 
  * p.init();
  * p.play();
  * while(true);
  */
 
 return 0;
 
 
}
