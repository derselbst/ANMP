#ifndef SONGINFO_H
#define SONGINFO_H

#include <string>


/**
  * class SongInfo
  * 
  */

struct SongInfo
{

    // Constructors/Destructors
    //  

    /**
     * Empty Destructor
     */
    ~SongInfo ();

    // Static Public attributes
    //  

    // Public attributes
    //  

    string Title;
    string Artist;
    string Composer;
    string Year;
    string Genre;

};

#endif // SONGINFO_H
