
#ifndef PLAYLISTFACTORY_H
#define PLAYLISTFACTORY_H

#include <string>

/**
  * class PlaylistFactory
  * 
  */

class PlaylistFactory
{
public:

    // Constructors/Destructors
    //  


    /**
     * Empty Constructor
     */
    PlaylistFactory ();

    /**
     * Empty Destructor
     */
    virtual ~PlaylistFactory ();

    // Static Public attributes
    //  

    // Public attributes
    //  



    /**
     * @param  playlist
     * @param  filePaths
     */
    static void addSongs (IPlaylist playlist, vector& filePaths);


    /**
     * @param  playlist
     * @param  filePath
     * @param  offset
     */
    static void addSong (IPlaylist playlist, string filePath, string offset = "");

protected:

    // Static Protected attributes
    //  

    // Protected attributes
    //  

public:

protected:

public:

protected:


private:

    // Static Private attributes
    //  

    // Private attributes
    //  

public:

private:

public:

private:



    /**
     * @return core::tree
     * @param  p
     */
    static core::tree getLoopsFromPCM (PCMHolder* p);


    /**
     * @return PCMHolder*
     * @param  extension
     */
    static PCMHolder* getPCMHolderFromFileType (string extension);


};

#endif // PLAYLISTFACTORY_H
