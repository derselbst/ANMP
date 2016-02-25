#ifndef SONG_H
#define SONG_H

#include <string>

#include "PCMHolder.h"
#include "tree.h"
#include "SongInfo.h"

using namespace std;

/**
  * class Song
  * 
  */

class Song;
bool operator< (const Song& lhs, const Song& rhs);

class Song
{
public:

    // Constructors/Destructors
    //  


    Song (PCMHolder* p, core::tree<loop_t> loops);

    virtual ~Song ();



    // Static Public attributes
    //  

    // Public attributes
    //  

    string& Filename;
    PCMHolder* pcm = nullptr;
    SongInfo Metadata;
    core::tree<loop_t> loops;


    /**
     * called to check whether the current song is playable or not
     * @return bool
     */
    bool isPlayable ();

};

#endif // SONG_H
