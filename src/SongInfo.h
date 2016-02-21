
#ifndef SONGINFO_H
#define SONGINFO_H

#include <string>
#include vector



/**
  * class SongInfo
  * 
  */

class SongInfo
{
public:

    // Constructors/Destructors
    //  


    /**
     * Empty Constructor
     */
    SongInfo ();

    /**
     * Empty Destructor
     */
    virtual ~SongInfo ();

    // Static Public attributes
    //  

    // Public attributes
    //  

    string Title;
    string Artist;
    string Composer;
    string Year;
    string Genre;

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

#endif // SONGINFO_H
