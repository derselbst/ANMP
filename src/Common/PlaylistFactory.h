#ifndef PLAYLISTFACTORY_H
#define PLAYLISTFACTORY_H


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
    static void addSongs (IPlaylist& playlist, vector<Song>& filePaths);


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
    static core::tree getLoopsFromPCM (PCMHolder* p);

};

#endif // PLAYLISTFACTORY_H
