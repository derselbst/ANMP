
#pragma once

#include "StandardWrapper.h"

// libsmf - helper lib for reading midi files
#include <smf.h>

constexpr int NMidiChannels = 16;

typedef struct _fluid_event_t fluid_event_t;
typedef struct _fluid_sequencer_t fluid_sequencer_t;
class FluidsynthWrapper;

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

    // time indexes in pulses
    Nullable<int> start_tick;
    Nullable<int> stop_tick;

    // how often this loop is repeated, 0 for infinite loops
    // specified by MIDI CC104
    uint8_t count = 0;

    // pointer to events which are part of this midi track loop
    std::vector<smf_event_t*> eventsInLoop;
};

/**
  * class MidiWrapper
  * 
  * a wrapper around libsmf, to support reading Standard Midi Files
  *
  */
class MidiWrapper : public StandardWrapper<float>
{
    public:
    MidiWrapper(string filename);
    MidiWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);


    // forbid copying
    MidiWrapper(MidiWrapper const &) = delete;
    MidiWrapper &operator=(MidiWrapper const &) = delete;

    ~MidiWrapper() override;

    static string SmfEventToString(smf_event_t *event);
    static void FluidSeqCallback(unsigned int time, fluid_event_t* e, fluid_sequencer_t* seq, void* data);

    // interface methods declaration

    void open() override;

    void close() noexcept override;

    frame_t getFrames() const override;

    void render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender) override;

    vector<loop_t> getLoopArray() const noexcept override;

    private:
    smf_t *smf = nullptr;
    FluidsynthWrapper *synth = nullptr;
    int lastOverridingLoopCount;
    bool lastUseLoopInfo;

    // first, outermost dimension: no. of the midi track
    // second dim: midi channel
    // third, innermost dim: id of the loop within that track
    vector<vector<vector<MidiLoopInfo>>> trackLoops;

    void initAttr();
    void initialize();
    void parseEvents();
    const MidiLoopInfo* getLongestMidiTrackLoop() const;
};
