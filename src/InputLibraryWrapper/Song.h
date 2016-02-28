
#ifndef PCMHOLDER_H
#define PCMHOLDER_H

#include <vector>
#include <string>

#include "types.h"
#include "tree.h"
#include "SongFormat.h"
#include "SongInfo.h"

using namespace std;

/**
  * class Song
  *
  */

class Song
{
protected:
    // even if there were no pure virtual methods, allow
    // construction for child classes only
    Song();

public:

    // empty virtual destructor for proper cleanup
    virtual ~Song();

    // forbid copying
    Song(Song const&) = delete;
    Song& operator=(Song const&) = delete;



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

    // holds metadata for this song e.g. title, interpret, album
    SongInfo Metadata;
    
    // a tree, that holds to loops to be played
    core::tree<loop_t> loopTree;

    /**
     * called to check whether the current song is playable or not
     * @return bool
     */
    bool isPlayable ();
    
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
    void buildLoopTree ();


    /**
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    virtual unsigned int getFrames () const = 0;

private:
  
    /**
     * @return vector
     */
    virtual vector<loop_t> getLoopArray () const = 0;
    
    
    static bool myLoopSort(loop_t i,loop_t j);
    
};

#endif // PCMHOLDER_H
