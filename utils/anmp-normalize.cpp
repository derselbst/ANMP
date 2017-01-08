// anmp-normalize: a tool for generating loudness normalization files for each song file.
// These are called .ebur128 files, due to normalization standard being used: https://tech.ebu.ch/docs/r/r128.pdf
// NOTE: you should only use this program for songs where really necessary (e.g. far too slient ones). this is because
// some artists intend to have silent songs. raising the volume of those songs would make the transition to the next song
// sound ugly.

#include <iostream>
#include <string>

#include <anmp.hpp>

#include <experimental/filesystem>
#include <thread>
#include <chrono>

using namespace std;
using namespace std::experimental::filesystem;

void onSongChanged(void* pthis)
{
  Song* s = static_cast<Playlist*>(pthis)->current();
  
  CLOG(LogLevel::INFO, "Now handling: " << s->Filename);
}


int main(int argc, char* argv[])
{
    gConfig.audioDriver = ebur128;
    gConfig.useLoopInfo = false;
    gConfig.RenderWholeSong = false;
    gConfig.useAudioNormalization = false;
    
    // this should be set to true since it significantly speeds up decoding
    // however with HLE a bit more silent signal might be generated, resulting in clipping, when listened to the accurate signal generated by LLE
    gConfig.useHle = false;


    constexpr int Threads = 4;
    Playlist plist[Threads];
    
    int curThread = 0;
    
    for(int i=1; i<argc; i++)
    {
        for (directory_entry dirEntry : recursive_directory_iterator(argv[i]))
        {
            if(is_regular_file(dirEntry.status()))
            {
                PlaylistFactory::addSong(plist[curThread], dirEntry.path());
                curThread = (curThread+1) % Threads;
            }
        }
    }

    
    std::vector<Player> players;
//     players.reserve(Threads);

    for(int i=0;i<Threads; ++i)
    {
        // terminate when reaching end of playlist
        plist[i].add(nullptr);
        
        Player p(&plist[i]);
        players.push_back(std::move(p));
        
        players.back().onCurrentSongChanged += make_pair(&plist[i], &onSongChanged);
    }
    
    for(int i=0;i<Threads; ++i)
    {
        players[i].play();
    }
    
    for(int i=0;i<Threads; ++i)
    {
        while(players[i].getIsPlaying())
        {
            this_thread::sleep_for(chrono::seconds(1));
        }
    }

    return 0;


}
