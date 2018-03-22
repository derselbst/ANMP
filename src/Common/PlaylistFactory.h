#ifndef PLAYLISTFACTORY_H
#define PLAYLISTFACTORY_H

#include "Nullable.h"
#include "SongInfo.h"
#include <string>

using namespace std;

class Song;
class IPlaylist;


/**
  * class PlaylistFactory
  *
  * a helper class to aid filling up a playlist
  */

class PlaylistFactory
{
    public:
    // no object
    PlaylistFactory() = delete;
    // no copy
    PlaylistFactory(const PlaylistFactory &) = delete;
    // no assign
    PlaylistFactory &operator=(const PlaylistFactory &) = delete;


    /**
     * try to add a song to the playlist
     * 
     * @param  playlist the playlist object the songs get added to
     * @param  filePath full path to an audio file on your HDD
     * @param  offset see Song::fileOffset
     * @param  len see Song::fileLen
     * @param  overridingMetadata not the metadata from Song::buildMetadata() but the metadata specified here will be used
     */
    static bool addSong(IPlaylist &playlist,
                        const string& filePath,
                        Nullable<size_t> offset = Nullable<size_t>(),
                        Nullable<size_t> len = Nullable<size_t>(),
                        Nullable<SongInfo> overridingMetadata = Nullable<SongInfo>());

#ifdef USE_CUE
    static void parseCue(IPlaylist &playlist, const string &filePath);
#endif

    template<typename T>
    static void tryWith(Song *(&pcm), const string &filePath, Nullable<size_t> offset, Nullable<size_t> len);
};


#include "PlaylistFactory_impl.h"


#endif // PLAYLISTFACTORY_H
