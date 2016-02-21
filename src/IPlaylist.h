
#ifndef IPLAYLIST_H
#define IPLAYLIST_H
#include "Config.h"

#include <string>
#include vector



/**
  * class IPlaylist
  * Holds the songs to be played
  */

class IPlaylist : virtual public Config
{
public:

    // Constructors/Destructors
    //  


    /**
     * Empty Constructor
     */
    IPlaylist ();

    /**
     * Empty Destructor
     */
    virtual ~IPlaylist ();

    // Static Public attributes
    //  

    // Public attributes
    //  



    /**
     * @param  song
     */
    virtual void add (Song song);


    /**
     */
    virtual void remove ();


    /**
     */
    virtual void next ();


    /**
     */
    virtual void previous ();


    /**
     */
    virtual void current ();

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



};

#endif // IPLAYLIST_H
