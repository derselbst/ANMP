#ifndef PLAYLISTFACTORY_H
#define PLAYLISTFACTORY_H

#include <string>
#include "Nullable.h"

using namespace std;

class Song;
class IPlaylist;


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
    static Song* addSong (IPlaylist& playlist, const string filePath, Nullable<size_t> offset = Nullable<size_t>(), Nullable<size_t> len = Nullable<size_t>());

#ifdef USE_CUE
    static void parseCue (IPlaylist& playlist, const string&filePath);
#endif
};

#endif // PLAYLISTFACTORY_H
