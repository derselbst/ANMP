
#ifndef SONGFORMAT_H
#define SONGFORMAT_H

#include <string>
#include vector



/**
  * class SongFormat
  * 
  */

class SongFormat
{
public:

    // Constructors/Destructors
    //  


    /**
     * Empty Constructor
     */
    SongFormat ();

    /**
     * Empty Destructor
     */
    virtual ~SongFormat ();

    // Static Public attributes
    //  

    // Public attributes
    //  

    unsigned int SampleRate;
    // indicates the sample format in pcm buffer "data"
    SampleFormat_t SampleFormat;
    int Channels;


    /**
     * returns bitrate in bit/s
     */
    void getBitrate ();

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

#endif // SONGFORMAT_H
