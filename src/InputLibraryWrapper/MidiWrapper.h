
#pragma once

#include "StandardWrapper.h"

#include "MidiFile.h"

#include <cstddef>
#include <vector>

struct smf_struct;
struct smf_track_struct;

struct smf_event_struct
{
    struct smf_track_struct *track = nullptr;
    int event_number = 0;
    int track_number = 0;
    int time_pulses = 0;
    double time_seconds = 0.0;
    std::vector<unsigned char> midi_buffer;
    std::size_t midi_buffer_length = 0;
};
typedef struct smf_event_struct smf_event_t;

struct smf_track_struct
{
    smf_struct *smf = nullptr;
    std::vector<smf_event_struct> events;
    int number_of_events = 0;
};

struct smf_struct
{
    smf::MidiFile midiFile;
    std::vector<smf_track_struct> tracks;
    std::vector<smf_event_t *> eventOrder;
    std::size_t iteratorIndex = 0;
    int ppqn = 0;
    int number_of_tracks = 0;
};
typedef struct smf_struct smf_t;

typedef struct _fluid_event_t fluid_event_t;
typedef struct _fluid_sequencer_t fluid_sequencer_t;
class FluidsynthWrapper;

struct MidiLoopInfo
{
    // the track this loop is valid for
    // same as event->track_number, i.e. one based
    int trackId;

    // unique id of this midi event given by the midi parser
    int eventId;

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
  * a wrapper around midifile, to support reading Standard Midi Files
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
    // second, innermost dim: id of the loop within that track
    vector<vector<MidiLoopInfo>> trackLoops;

    void initAttr();
    void initialize();
    void parseEvents();
    const MidiLoopInfo* getLongestMidiTrackLoop() const;
};

inline bool smf_event_is_valid(const smf_event_t *event)
{
    return event != nullptr && !event->midi_buffer.empty();
}

inline bool smf_event_is_metadata(const smf_event_t *event)
{
    return smf_event_is_valid(event) && event->midi_buffer[0] == 0xFF;
}

inline bool smf_event_is_sysex(const smf_event_t *event)
{
    return smf_event_is_valid(event) && (event->midi_buffer[0] == 0xF0 || event->midi_buffer[0] == 0xF7);
}
