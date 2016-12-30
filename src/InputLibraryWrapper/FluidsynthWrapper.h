
#pragma once

#include "StandardWrapper.h"

#include <fluidsynth.h>

// libsmf - helper lib for reading midi files
#include <smf.h>

/**
  * class FluidsynthWrapper
  *
  */


class FluidsynthWrapper : public StandardWrapper<float>
{
public:
    FluidsynthWrapper(string filename);
    FluidsynthWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);


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
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    frame_t getFrames () const override;

    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;

    vector<loop_t> getLoopArray () const noexcept override;

private:
    smf_t* smf = nullptr;
    fluid_settings_t* settings = nullptr;
    fluid_synth_t* synth = nullptr;
    fluid_sequencer_t* sequencer = nullptr;

    short synthSeqId, mySeqID;


    struct MidiLoopInfo
    {
        // the track this loop is valid for
        // same as event->track_number, i.e. one based
        int trackId;
        
        // unique id of this midi event given by libsmf
        int eventId;
        
        // the channel this loop is valid for
        // same as event->channel
        uint8_t channel;
        
        // unique id of this loop, as specified by value of MIDI CC102 and CC103
        uint8_t loopId;
        
        // time indexes in seconds
        Nullable<double> start;
        Nullable<double> stop;
        
        // how often this loop is repeated, 0 for infinite loops
        // specified by MIDI CC104
        uint8_t count = 0;
    };

    // first dimension: no. of the midi track
    // second dim: midi channel
    // third dim: id of the loop within that track
    vector< vector< vector<MidiLoopInfo> > > trackLoops;


    static string SmfEventToString(smf_event_t* event);
    static void scheduleTrackLoop(unsigned int time, fluid_event_t* e, fluid_sequencer_t* seq, void* data);

    void initAttr();
    void setupSettings();
    void setupSynth();
    void setupSeq();

    int scheduleNextCallback(smf_event_t* event, unsigned int time, void* data);
    void feedToFluidSeq(smf_event_t * event, fluid_event_t* fluidEvt, double offset);
};
