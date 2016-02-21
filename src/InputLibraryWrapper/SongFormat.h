#ifndef SONGFORMAT_H
#define SONGFORMAT_H


/**
  * class SongFormat
  * 
  */

struct SongFormat
{
    unsigned int SampleRate=0;
    // indicates the sample format in pcm buffer "data"
    SampleFormat_t SampleFormat=SampleFormat_t::unset;
    int Channels=0;
    
    // Constructors/Destructors
    //  

    /**
     * Empty Destructor
     */
    ~SongFormat ();

    /**
     * returns bitrate in bit/s
     */
    void getBitrate ();

};

#endif // SONGFORMAT_H
