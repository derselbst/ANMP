
#ifndef PCMHOLDER_H
#define PCMHOLDER_H

#include <vector>
#include <string>

#include "types.h"
#include "Nullable.h"
#include "tree.h"
#include "SongFormat.h"
#include "SongInfo.h"

using namespace std;

/**
  * @brief abstract class Song
  *
  * base class for all the different wrapper classes, holding all information that we need to support decoding various audio formats and properly playing them back
  *
  * workflow:
  *    need to support a new audio format?
  *    -> find a library that supports decoding this format!
  *    -> to make this library usable for ANMP: write a wrapper class by deriving this class (or StandardWrapper), implementing all abstract methods
  *    -> update PlaylistFactory accordinly to use the newly written wrapper lib
  *    -> listen audio and enjoy
  */
class Song
{
protected:
    // even if there were no pure virtual methods, allow
    // construction for child classes only
    Song(string filename);
    Song(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);

public:

    // empty virtual destructor for proper cleanup
    virtual ~Song() = default;

    // forbid copying
    Song(Song const&) = delete;
    Song& operator=(Song const&) = delete;

//--------------------------------------------------------------------
// File specific level
//--------------------------------------------------------------------
    // fullpath to underlying audio file
    string Filename = "";

    // an in-file-offset. its usage can be determined by child classes
    //
    // usual usage: how many milliseconds to skip when reading pcm from file
    //
    // or it can be used as sub-song-offset (if one file contains multiple files)
    Nullable<size_t> fileOffset;

    // how many milliseconds following "fileOffset" shall be used for playing
    Nullable<size_t> fileLen;
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// RAW PCM Buffer specific area
//--------------------------------------------------------------------
    // points to at least "gConfig.FramesToRender" freshly rendered PCM frames,
    // but usually contains as many as "this->getFrames()" frames of PCM
    // 
    // PCM will have its channels interleaved, i.e.:
    //    __________________________________________________________________
    //    ||                      FRAME 0                  ||    FRAME 1   ||   
    //    ||  Item for   |  Item  for  | ... |  Item  for  ||   (...dito)  ||
    //    ||  channel 0  |  channel 1  | ... |  channel n  ||              ||
    //
    // item is an alias for sample
    pcm_t* data = nullptr;

    // indicates the data type of the raw decoded pcm
    SongFormat Format;

    // how many items (i.e. floats, int16s, etc.) are there in this->data?
    size_t count = 0;

    // a tree, that holds the loops to be played
    /*  example: syntax aggreement: ([a,b],k) defines an instance of loop_t where:
        a: loop_t::start
        b: loop_t::stop
        k: loop_t::count
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

    // holds metadata for this song e.g. title, interpret, album
    SongInfo Metadata;

    /**
     * opens the current file using the corresponding lib
     *
     * specificly: defines samplerate, defines channelcount, provides a value for this->getFrames()
     *
     * @exceptions throws runtime_error if file cannot be opened, is a file of unsupported format or smth. else that is crucial for decoding goes wrong
     *
     * @note everytime this function returns (i.e. by NOT throwing an exception), it is assumed that the file tried to open can be decoded with that specific wrapper class
     */
    virtual void open () = 0;

    /**
     * frees all ressources acquired by this->open()
     *
     * shall always be called after a call to this->open(), even if this->open() threw an exception
     *
     * multiple calls to this method (i.e. without corresponding calls to this->open()) shall be possible, leading to no error
     *
     * thus this method should at least be called by the destructor
     */
    virtual void close () noexcept = 0 ;

    /** fills the pcm buffer this->data, preparing atleast "gConfig.FramesToRender" PCM frames
     *
     * synchronous part: allocates the pcm buffer and fills it up to have enough for gConfig.PreRenderTime time of playback
     * asynchronous part: fills rest of pcm buffer
     */
    virtual void fillBuffer () = 0;

    /**
     * frees all ressources acquired by this->fillBuffer()
     *
     * multiple calls to this method (i.e. without corresponding calls to this->fillBuffer()) shall be possible, leading to no error
     */
    virtual void releaseBuffer () noexcept = 0;

    /**
     * gathers the song metadata and populates this->Metadata
     */
    virtual void buildMetadata () noexcept;

    /**
     * returns an unsorted array of loops that could be found in this->Filename. Its only valid to call this method while the song is this->open(). Though it may work after calling this->close().
     * 
     * the song has no loops? --> return an empty vector
     */
    virtual vector<loop_t> getLoopArray () const noexcept;


    /**
     * gets the number of audio frames that are necessary to play the whole song once without respecting any loops!
     * 
     * those frames dont necessarily have to be in the pcm buffer all at one time
     *
     * this method mustn't return zero!
     *
     * @return an unsigned integer (even if frame_t is signed) greater 1
     */
    virtual frame_t getFrames () const = 0;


    /**
     * public helper method for building up the this->loopTree, by requesting looparrays via this->getLoopArray()
     */
    void buildLoopTree();

    // TODO: REMOVE ME? or better really implement and use me?
    bool isPlayable() noexcept;

private:
    static bool myLoopSort(loop_t i,loop_t j);
    static bool loopsMatch(const loop_t& parent, const loop_t& child);
    static core::tree<loop_t>& findRootLoopNode(core::tree<loop_t>& loopTree, const loop_t& subLoop);

};

#endif // PCMHOLDER_H
