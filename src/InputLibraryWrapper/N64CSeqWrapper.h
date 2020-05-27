
#pragma once

#include "FluidsynthWrapper.h"
#include "StandardWrapper.h"

#include <stack>

enum MidiEvt
{
    /* For distinguishing channel number from status */
    MIDI_ChannelMask = 0x0F,
    MIDI_StatusMask = 0xF0,

    /* Channel voice messages */
    MIDI_NoteOff = 0x80,
    MIDI_NoteOn = 0x90,
    MIDI_PolyKeyPressure = 0xA0,
    MIDI_ControlChange = 0xB0,
    MIDI_ChannelModeSelect = 0xB0,
    MIDI_ProgramChange = 0xC0,
    MIDI_ChannelPressure = 0xD0,
    MIDI_PitchBendChange = 0xE0,

    MIDI_Meta = 0xFF
};

enum MidiCtrl
{
    MIDI_VOLUME_CTRL = 0x07,
    MIDI_PAN_CTRL = 0x0A,
    MIDI_PRIORITY_CTRL = 0x10,
    MIDI_SUSTAIN_CTRL = 0x40,
    MIDI_FX1_CTRL = 0x5B,
    MIDI_FX3_CTRL = 0x5D
};

enum MidiMeta
{
    MIDI_META_TEMPO = 0x51,
    MIDI_META_EOT = 0x2f,
    CMIDI_BLOCK_CODE = 0xFE,
    CMIDI_LOOPSTART_CODE = 0x2E,
    CMIDI_LOOPEND_CODE = 0x2D
};

enum class Evt : int16_t
{
    MIDI,
    TEMPO,
    SEQ_END,
    TRACK_END,
    LOOPSTART,
    LOOPEND
};


struct CSeqState
{
    uint32_t trackOffset[NMidiChannels];
    uint32_t validTracks; /* set of flags, showing valid tracks    */
    uint32_t division; /* ticks / qrter notes  */
    uint32_t lastTicks; /* keep track of ticks incase app wants  */
    uint32_t lastTempo; /* latest tempo  */
    float lastMSec; /* latest milli seconds  */
    uint32_t lastDeltaTicks; /* number of delta ticks of last event   */
    bool deltaFlag; /* flag: set if delta's not subtracted   */
    uint8_t *cur[NMidiChannels]; /* ptr to current track location, may point to next event, or may point to a backup code */
    uint8_t *curBckp[NMidiChannels]; /* ptr to next event if in backup mode   */
    uint8_t curBckLen[NMidiChannels]; /* if > 0, then in backup mode      */
    uint8_t lastStatus[NMidiChannels]; /* for running status     */
    uint32_t evtDeltaTicks[NMidiChannels]; /* delta time to next event    */
    std::array<std::stack<uint16_t>, NMidiChannels> lastLoopStartId{};
};

struct MidiEvent
{
    uint8_t status;
    uint8_t byte1;
    uint8_t byte2;
    uint32_t duration;
};

struct TempoEvent
{
    uint8_t status;
    uint8_t type;
    uint8_t len;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
};

struct LoopEvent
{
    uint32_t track; // track ID this loop belongs to
    uint16_t id;
    uint16_t count;
};

struct CSeqEvent
{
    Evt type;
    int32_t ticks;
    union
    {
        MidiEvent midi;
        TempoEvent tempo;
        LoopEvent loop;
    } msg;
};

// old-school CSeq Midi header with 16 hardcoded track offsets
struct CMidiHdr
{
    uint32_t trackOffset[16];
    uint32_t division;
};

// variable length header as found in BanjoTooie
struct BTMidiHdr
{
    uint32_t division;
    uint32_t totalTracks;
};

constexpr int MaxFileSize = 1024 * 1024;

struct CSeqLoopInfo
{
    // the track this loop is valid for (zero-based)
    int trackId;

    int loopId;

    int count;

    // time indexes in seconds
    Nullable<uint32_t> start;
    Nullable<uint32_t> stop;

    // time indexes in pulses
    Nullable<uint32_t> start_tick;
    Nullable<uint32_t> stop_tick;
};

class N64CSeqWrapper : public StandardWrapper<float>
{
    public:
    N64CSeqWrapper(string filename);
    N64CSeqWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);


    // forbid copying
    N64CSeqWrapper(N64CSeqWrapper const &) = delete;
    N64CSeqWrapper &operator=(N64CSeqWrapper const &) = delete;

    ~N64CSeqWrapper() override;

    // interface methods declaration

    void open() override;

    void close() noexcept override;

    frame_t getFrames() const override;

    void render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender) override;

    vector<loop_t> getLoopArray() const noexcept override;

    static void SequencerCallback(unsigned int time, fluid_event_t *e, fluid_sequencer_t *seq, void *data);

    private:
    std::vector<unsigned char> fileBuf;
    CSeqState seq;
    fluid_event_t *evt = nullptr;
    FluidsynthWrapper *synth = nullptr;
    int lastOverridingLoopCount;
    bool lastUseLoopInfo;

    // first, outermost dimension: no. of the midi track
    // second, innermost dim: id of the loop within that track
    std::array<std::vector<CSeqLoopInfo>, NMidiChannels> trackLoops{};

    void initAttr();
    void parseFirstTime(CSeqState *seq);
    const CSeqLoopInfo *getLongestMidiTrackLoop() const;

    uint32_t cSeqGetTrackEvent(CSeqState *seq, uint32_t track, CSeqEvent *event, bool ignoreLoops);
    uint32_t cSeqNextEvent(CSeqState *seq, CSeqEvent *evt, bool ignoreLoops);
    void handleNextSeqEvent(CSeqState *seq, CSeqEvent *evt, uint32_t track);
    void handleMIDIMsg(CSeqState *seq, CSeqEvent *event, uint32_t track);
    void handleMetaMsg(CSeqState *seq, CSeqEvent *event, bool dispatchEvent);
};
