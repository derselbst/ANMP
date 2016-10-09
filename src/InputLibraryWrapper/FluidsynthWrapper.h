
#pragma once

#include "StandardWrapper.h"

#include <fluidsynth.h>


/**
  * class FluidsynthWrapper
  *
  */

class FluidsynthWrapper : public StandardWrapper<float>
{
public:
    FluidsynthWrapper(string filename, string soundfont);
//     FluidsynthWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    

    // forbid copying
    FluidsynthWrapper(FluidsynthWrapper const&) = delete;
    FluidsynthWrapper& operator=(FluidsynthWrapper const&) = delete;

    virtual ~FluidsynthWrapper ();

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
     * @return vector
     */
    vector<loop_t> getLoopArray () const noexcept override;


    /**
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    frame_t getFrames () const override;

    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;

    void buildMetadata() noexcept override;
private:
        static frame_t backupFramesToRender;
        
        fluid_settings_t* settings = nullptr;
        fluid_synth_t* synth = nullptr;
        fluid_player_t* player = nullptr;
        
        // path to the soundfont to use
        string soundfontFile = "";
        
        
        void initAttr(string soundfont);
        void dryRun();

};
