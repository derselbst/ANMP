#ifndef LIBSNDWRAPPER_H
#define LIBSNDWRAPPER_H

#include <sndfile.h>

#include "Song.h"


/**
  * class LibSNDWrapper
  *
  */

class LibSNDWrapper : public Song
{
public:

    // Constructors/Destructors
    //


    /**
     * Empty Constructor
     */
    LibSNDWrapper (string filename, size_t offset=0);

    /**
     * Empty Destructor
     */
    virtual ~LibSNDWrapper ();

    // interface methods declaration

    /**
     * opens the current file using the corresponding lib
     */
    void open () override;


    /**
     */
    void close () override;


    /** synchronos read call to library goes here
     */
    void fillBuffer () override;


    /**
     */
    void releaseBuffer () override;


    /**
     * @return vector
     */
    vector<loop_t> getLoopArray () const;


    /**
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    unsigned int getFrames () const override;

private:
    SNDFILE *sndfile = nullptr;
    SF_INFO sfinfo;

};

#endif // LIBSNDWRAPPER_H
