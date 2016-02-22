#ifndef SONG_H
#define SONG_H

#include <string>


/**
  * class Song
  * 
  */

class Song
{
public:

    // Constructors/Destructors
    //  


    Song (PCMHolder* p);

    virtual ~Song ();

    // Static Public attributes
    //  

    // Public attributes
    //  

    string& Filename;
    PCMHolder* pcm = nullptr;
    SongInfo Metadata;
    core::tree loops;


    /**
     * called to check whether the current song is playable or not
     * @return bool
     */
    bool isPlayable ();

};

#endif // SONG_H
