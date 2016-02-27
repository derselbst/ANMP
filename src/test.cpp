#include <iostream>
#include <string>
#include "unistd.h"

#include "Playlist.h"
#include "PlaylistFactory.h"
#include "Player.h"

using namespace std;

int main()
{
 vector<string> filenames;
filenames.push_back("somewav.wav");
filenames.push_back("someother.wav");
 
 
 IPlaylist* plist = new Playlist();
 
 PlaylistFactory::addSongs(*plist, filenames);
 
 Player p(plist);

   
   p.init();
   p.play();
      sleep(2);
   cout << "seek" <<endl;
   p.seekTo(200000);

   sleep(10);
cout << "stop" <<endl;
   p.stop();
   cout << "play" <<endl;
  p.play();
  cout << "achtung" <<endl;
    sleep(5);
    cout << "pause" <<endl;
   p.pause();
   p.play();
   sleep(5);
 return 0;
 
 
}
