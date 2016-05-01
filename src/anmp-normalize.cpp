#include <iostream>
#include <string>

#include "unistd.h"

#include "types.h"
#include "Playlist.h"
#include "PlaylistFactory.h"
#include "Player.h"
#include "Config.h"


using namespace std;

int main()
{
    Config::audioDriver = ebur128;
    Config::useLoopInfo = false;
    Config::RenderWholeSong = false;
    
    vector<string> filenames;
    filenames.push_back("something");

    IPlaylist* plist = new Playlist();

    for(int i=0; i<filenames.size(); i++)
    {
        PlaylistFactory::addSong(*plist, filenames[i]);
    }

    Player p(plist);


    p.play();

    while(p.getIsPlaying())
    {
        sleep(1);
    }

    delete plist;

    return 0;


}
