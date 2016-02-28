#ifndef PLAYLISTFACTORY_H
#define PLAYLISTFACTORY_H

#include "IPlaylist.h"

#include <vector>
#include <string>

using namespace std;

/**
  * class PlaylistFactory
  *
  */

class PlaylistFactory
{
public:

    // no object
    PlaylistFactory() = delete;
    // no copy
    PlaylistFactory(const PlaylistFactory&) = delete;
    // no assign
    PlaylistFactory& operator=(const PlaylistFactory&) = delete;



    /**
     * @param  playlist
     * @param  filePaths
     */
    static void addSongs (IPlaylist& playlist, vector<string>& filePaths);


    /**
     * @param  playlist
     * @param  filePath
     * @param  offset
     */
    static void addSong (IPlaylist& playlist, string filePath, string offset = "");


};

#endif // PLAYLISTFACTORY_H
