
#ifndef PCMHOLDER_H
#define PCMHOLDER_H

#include <vector>
#include <string>

#include "types.h"
#include "SongFormat.h"

using namespace std;

/**
  * class PCMHolder
  * 
  */

class PCMHolder
{
protected:
    // even if there were no pure virtual methods, allow
    // construction for child classes only
    PCMHolder(){};

public:

    // empty virtual destructor for proper cleanup
    virtual ~PCMHolder(){};

    // forbid copying
    PCMHolder(PCMHolder const&) = delete;
    PCMHolder& operator=(PCMHolder const&) = delete;



    // pointer to pcm data
    pcm_t* data = nullptr;
    
    // how many items (i.e. floats, int16s, etc.) are there in data?
    size_t count = 0;
    
    // pcm specific information
    SongFormat Format;
    
    // how many frames to skip when reading pcm from file
    size_t offset;
    
    // fullpath to underlying audio file
    string Filename;



    /**
     * opens the current file using the corresponding lib
     */
    virtual void open () = 0;


    /**
     */
    virtual void close () = 0;


    /** synchronos read call to library goes here
     */
    virtual void fillBuffer () = 0;


    /**
     */
    virtual void releaseBuffer () = 0;


    /**
     * @return vector
     */
    virtual vector<loop_t> getLoops () const = 0;


    /**
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    virtual unsigned int getFrames () const = 0;

};

#endif // PCMHOLDER_H
