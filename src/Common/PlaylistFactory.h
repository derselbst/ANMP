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
     * @param  filePath
     * @param  offset
     */
    static Song* addSong (IPlaylist& playlist, const string filePath, size_t offset = 0, size_t len = 0);
    static void parseCue (IPlaylist& playlist, const string&filePath);

};

#endif // PLAYLISTFACTORY_H
