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

    for(int i=0; i<filenames.size(); i++)
    {
        PlaylistFactory::addSong(*plist, filenames[i]);
    }

    Player p(plist);


    p.play();

    /*p.init();
    cout << "seek" <<endl;p.play();
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
    p.play();*/
    while(1)
    {
        sleep(5);
    }

    delete plist;

    return 0;


}
