// anmp-normalize: a tool for generating loudness normalization files for each song file.
// These are called .ebur128 files, due to normalization standard being used: https://tech.ebu.ch/docs/r/r128.pdf
// NOTE: you should only use this program for songs where really necessary (e.g. far too slient ones). this is because
// some artists intend to have silent songs. raising the volume of those songs would make the transition to the next song
// sound ugly.

#include <iostream>
#include <string>

// #include "unistd.h"

#include "types.h"
#include "Playlist.h"
#include "PlaylistFactory.h"
#include "Player.h"
#include "Song.h"
#include "Config.h"



#include <experimental/filesystem>
#include <thread>
#include <chrono>

using namespace std;
using namespace std::experimental::filesystem;

IPlaylist* plist;

void onSongChanged(void* pthis)
{
  (void) pthis;
  
  Song* s = plist->current();
  
  cout << "Now handling: " << s->Filename << endl;
}


int main(int argc, char* argv[])
{
    Config::audioDriver = ebur128;
    Config::useLoopInfo = false;
    Config::RenderWholeSong = false;
    Config::useAudioNormalization = false;
    Config::useHle = true;


    plist = new Playlist();
    
    for(int i=1; i<argc; i++)
    {
      for (directory_entry dirEntry : recursive_directory_iterator(argv[i]))
      {
        PlaylistFactory::addSong(*plist, dirEntry.path());
      }
    }

    // terminate when reaching end of playlist
    plist->add(nullptr);


    Player p(plist);
    p.onCurrentSongChanged += make_pair(nullptr, &onSongChanged);
    p.play();

    while(p.getIsPlaying())
    {
      this_thread::sleep_for(chrono::seconds(1));
    }

    delete plist;

    return 0;


}
