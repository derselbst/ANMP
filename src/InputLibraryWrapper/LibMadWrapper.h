#ifndef LIBMADWRAPPER_H
#define LIBMADWRAPPER_H

#include "StandardWrapper.h"

#include <mad.h>
#include <future>


/**
  * class LibMadWrapper
  *
  */

class LibMadWrapper : public StandardWrapper<int32_t>
{
public:
    LibMadWrapper(string filename);
    LibMadWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

    // forbid copying
    LibMadWrapper(LibMadWrapper const&) = delete;
    LibMadWrapper& operator=(LibMadWrapper const&) = delete;

    virtual ~LibMadWrapper ();

    // interface methods declaration

    /**
     * opens the current file using the corresponding lib
     */
    void open () override;


    /**
     */
    void close () noexcept override;


    /** PCM buffer fill call to underlying library goes here
     */
    void fillBuffer () override;

    /**
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    frame_t getFrames () const override;

    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;

    void buildMetadata() noexcept override;

private:
    FILE* infile = nullptr;
    unsigned char * mpegbuf = nullptr;
    int mpeglen = 0;
    struct mad_stream* stream = nullptr;
    
    // these two are only really needed inside this->render()
    // however, we need to preserve these two across subsequent calls to this->render(), since they are somewhat interdependent
    // not saving them would cause ugly interruptions in produced audio
    //
    // the header of frame might be reused?
    Nullable<struct mad_frame> frame;
    // and the member phase of synth is definitly reused
    Nullable<struct mad_synth> synth;
    
    vector<int32_t> tempBuf;

    frame_t numFrames=0;
    static signed int toInt24Sample(mad_fixed_t sample);
    static string id3_get_tag (struct id3_tag const *tag, char const *what);
    
    int findValidHeader(struct mad_header& header);

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
