#ifndef LIBSNDWRAPPER_H
#define LIBSNDWRAPPER_H

#include "StandardWrapper.h"

#include <sndfile.h>
#include <future>


/**
  * class LibSNDWrapper
  *
  */

class LibSNDWrapper : public StandardWrapper<int32_t>
{
public:

    // Constructors/Destructors
    //


    /**
     * Empty Constructor
     */
    LibSNDWrapper(string filename);
    LibSNDWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

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


    /** PCM buffer fill call to underlying library goes here
     */
    void fillBuffer () override;


    /**
     */
    void releaseBuffer () override;


    /**
     * @return vector
     */
    vector<loop_t> getLoopArray () const override;


    /**
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    frame_t getFrames () const override;

    void render(frame_t framesToRender) override;

    void buildMetadata() override;
private:
    SNDFILE* sndfile = nullptr;
    SF_INFO sfinfo;
};

#endif // LIBSNDWRAPPER_H
