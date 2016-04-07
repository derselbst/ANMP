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

    LibGMEWrapper(string filename, size_t fileOffset=0, size_t fileLen=0);

    /**
     * Empty Destructor
     */
    virtual ~LibGMEWrapper();

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
    
    vector<loop_t> getLoopArray () const override;

    void render(frame_t framesToRender) override;
    
    void buildMetadata() override;

private:
    // length in ms to fade
    unsigned long fade_ms = 0;

    Music_Emu * handle = nullptr;
    gme_info_t* info = nullptr;

    static void printWarning( Music_Emu* emu );

    bool wholeSong() const;


};

#endif // LIBGMEWRAPPER_H
