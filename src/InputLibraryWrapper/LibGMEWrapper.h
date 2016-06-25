#ifndef LIBGMEWRAPPER_H
#define LIBGMEWRAPPER_H

#include "StandardWrapper.h"

#include <future>
#include <gme.h>

/**
  * class LibGMEWrapper
  *
  */
class LibGMEWrapper : public StandardWrapper<int16_t>
{
public:

    // Constructors/Destructors
    //

    LibGMEWrapper(string filename);
    LibGMEWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();
    
    // forbid copying
    LibGMEWrapper(LibGMEWrapper const&) = delete;
    LibGMEWrapper& operator=(LibGMEWrapper const&) = delete;

    virtual ~LibGMEWrapper();

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

    vector<loop_t> getLoopArray () const noexcept override;

    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;

    void buildMetadata() noexcept override;

private:
    // length in ms to fade
    unsigned long fade_ms = 0;

    Music_Emu * handle = nullptr;
    gme_info_t* info = nullptr;

    static void printWarning( Music_Emu* emu );

    bool wholeSong() const;


};

#endif // LIBGMEWRAPPER_H
