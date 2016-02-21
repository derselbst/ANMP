
#ifndef SONG_H
#define SONG_H

#include <string>
#include vector



/**
  * class Song
  * 
  */

class Song
{
public:

    // Constructors/Destructors
    //  


    /**
     * Empty Constructor
     */
    Song ();

    /**
     * Empty Destructor
     */
    virtual ~Song ();

    // Static Public attributes
    //  

    // Public attributes
    //  

    string& Filename;
    PCMHolder* pcm;
    SongInfo Metadata;
    core::tree loops;


    /**
     * called to check whether the current song is playable or not
     * @return bool
     */
    bool isPlayable ();

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


    void initAttributes () ;

};

#endif // SONG_H
