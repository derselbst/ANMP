#ifndef LIBMADWRAPPER_H
#define LIBMADWRAPPER_H

#include "StandardWrapper.h"

#include <mad.h>
#include <future>


/**
  * class LibMadWrapper
  *
  */

class LibMadWrapper : public StandardWrapper<int16_t>
{
public:

    // Constructors/Destructors
    //


    /**
     * Empty Constructor
     */
    LibMadWrapper(string filename);
    LibMadWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

    /**
     * Empty Destructor
     */
    virtual ~LibMadWrapper ();

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
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    frame_t getFrames () const override;

    void render(frame_t framesToRender) override;

    void buildMetadata() override;

private:
    FILE* infile = nullptr;
    unsigned char * mpegbuf = nullptr;
    int mpeglen = 0;
    struct mad_stream stream;

    frame_t numFrames=0;
    static signed int toInt16Sample(mad_fixed_t sample);

};

/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

// struct buffer {
//   LibMadWrapper* context;
//   unsigned char const *start;
//   unsigned long length;
// };

#endif // LIBSNDWRAPPER_H
