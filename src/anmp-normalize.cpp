#include <iostream>
#include <string>

#include "unistd.h"

#include "types.h"
#include "Playlist.h"
#include "PlaylistFactory.h"
#include "Player.h"
#include "Config.h"



//     #include <experimental/filesystem>

using namespace std;
// using std::experimental::filesystem::recursive_directory_iterator;

int main(int argc, char* argv[])
{
    Config::audioDriver = ebur128;
    Config::useLoopInfo = false;
    Config::RenderWholeSong = false;
    Config::useAudioNormalization = false;
    Config::useHle = true;


// for (auto& dirEntry : recursive_directory_iterator(myPath))
//      cout << dirEntry << endl;
//
//     return  0;


    vector<string> filenames;
//     filenames.push_back("something");
    for(int i=1; i<argc; i++)
    {
        filenames.push_back(argv[i]);
    }

    IPlaylist* plist = new Playlist();

    for(unsigned int i=0; i<filenames.size(); i++)
    {
        PlaylistFactory::addSong(*plist, filenames[i]);
    }

    // terminate when reaching end of playlist
    plist->add(nullptr);


    Player p(plist);
    p.play();

    while(p.getIsPlaying())
    {
        sleep(1);
    }

    delete plist;

    return 0;


}
