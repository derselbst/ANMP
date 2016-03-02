
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
    Song(string filename, size_t offset=0);

public:

    // empty virtual destructor for proper cleanup
    virtual ~Song();

    // forbid copying
    Song(Song const&) = delete;
    Song& operator=(Song const&) = delete;

//--------------------------------------------------------------------
// File specific level    
//--------------------------------------------------------------------
    // fullpath to underlying audio file
    string Filename;
    
    // how many frames to skip when reading pcm from file
    size_t offset;
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// RAW PCM Buffer specific area
//--------------------------------------------------------------------
    // pointer to pcm data
    pcm_t* data = nullptr;

    // how many items (i.e. floats, int16s, etc.) are there in data?
    size_t count = 0;

    // pcm specific information
    SongFormat Format;

    // a tree, that holds to loops to be played
/*  example: syntax aggreement: ([a,b],k) defines an instance of loop_t where:
    a: loop_t::start
    b: loop_t::stop
    k  loop_t::count
    given is an audio file with N frames
    in order to make the audio file play, the root node of loopTree will always
    contain ([0,N],1)
    imagine that this audio file has additionally defined loop points (such as
    in smpl chunk in RIFF-WAVE files)
    
    this.getLoopArray() will return these loop points as array, based on that array,
    the loopTree is begin built, which might look like this:
    
                          ([0,N],1)
                        /      |   \
                      /        |     \
                    /          |       \
                  /            |         \
                /              |           \
              /                |             \
    ([3,10],5)          ([15,12000],6)      ([12001,65344],1)
                               |                     \
                               |                      \
                        ([20,4000],123)           ([12005, 12010],2)
                           /         \
                          /           \
                   ([20-22],4)     ([30,3000],23)
*/
    core::tree<loop_t> loopTree;
//--------------------------------------------------------------------


    // holds metadata for this song e.g. title, interpret, album
    SongInfo Metadata;
    


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
    virtual frame_t getFrames () const = 0;

private:
  
    /**
     * @return vector
     */
    virtual vector<loop_t> getLoopArray () const = 0;
    
    
    static bool myLoopSort(loop_t i,loop_t j);
    
};

#endif // PCMHOLDER_H
