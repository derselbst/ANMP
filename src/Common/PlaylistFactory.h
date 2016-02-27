#ifndef PLAYLISTFACTORY_H
#define PLAYLISTFACTORY_H

#include "IPlaylist.h"
#include "PCMHolder.h"
#include "tree.h"

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


private:

    /**
     * @return core::tree
     * @param  p
     */
    static core::tree<loop_t> getLoopFromPCM (PCMHolder* p);
    static bool myLoopSort(loop_t i,loop_t j);

};

#endif // PLAYLISTFACTORY_H
