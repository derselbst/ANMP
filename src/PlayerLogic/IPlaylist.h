#ifndef IPLAYLIST_H
#define IPLAYLIST_H


/**
  * class IPlaylist
  * Holds the songs to be played
  */

class IPlaylist
{
public:

    // Constructors/Destructors
    //  
    // Empty virtual destructor for proper cleanup
    virtual ~IPlaylist() {}


    /**
     * @param  song
     */
    virtual void add (Song song) = 0;


    /**
     */
    virtual void remove (Song song) = 0;
    
    /**
     */
    virtual void remove (int i) = 0;


    /**
     */
    virtual Song& next () = 0;


    /**
     */
    virtual Song& previous () = 0;


    /**
     */
    virtual Song& current () = 0;

};

#endif // IPLAYLIST_H
