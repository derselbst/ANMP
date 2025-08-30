
#pragma once

#include "MidiWrapper.h"

#include <fluidsynth.h>

// libsmf - helper lib for reading midi files
#include <smf.h>
#include <vector>
#include <memory>
#include <queue>

struct MidiNoteInfo;
struct ALSeqpLoopEvent;
class N64CSeqWrapper;

constexpr int NMidiChannels = 32;

/**
  * class FluidsynthWrapper
  *
  */
class FluidsynthWrapper
{
    public:
    FluidsynthWrapper();
    ~FluidsynthWrapper();
    void Init(const Nullable<string>& suggestedSf2, N64CSeqWrapper* cseq=nullptr);

    // forbid copying
    FluidsynthWrapper(FluidsynthWrapper const &) = delete;
    FluidsynthWrapper &operator=(FluidsynthWrapper const &) = delete;

    void InformHasNoteOn(int chan);
    void InformHasProgChange(int chan);
    void ConfigureChannels(SongFormat *f);
    void SetDefaultSeqTempoScale(unsigned int ppqn);

    // returns the samplerate that will be synthesized at
    unsigned int GetSampleRate();

    int GetActiveMidiChannels();
    int GetAudioVoices();
    int GetEffectVoices();
    int GetEffectCount();
    bool GetReverbActive();
    static constexpr int GetChannelsPerVoice();
    static double GetTempoScale(unsigned int uspqn, unsigned int ppqn);

    void AddEvent(smf_event_t *event, double offset = 0.0);
    void AddEvent(fluid_event_t *event, uint32_t tick);
    void ScheduleLoop(MidiLoopInfo *loopInfo);
    void ScheduleTempoChange(double newScale, int atTick, bool absolute);
    void ScheduleSysEx(smf_event_t*, int atTick, bool absolute);
    void FinishSong(int millisec);

    void Render(float *bufferToFill, frame_t framesToRender);

    private:
    fluid_settings_t *settings = nullptr;
    fluid_synth_t *synth = nullptr;
    fluid_sequencer_t *sequencer = nullptr;

    // common fluid_event_t used for most MIDI CCs and stuff
    fluid_event_t *synthEvent = nullptr;

    // event used for scheduling midi track loops and calling back MidiWrapper
    fluid_event_t *callbackEvent = nullptr;

    // event used for triggering note on/offs by calling back ourselfs
    fluid_event_t *callbackNoteEvent = nullptr;
    
    fluid_event_t *sysexEvent = nullptr;

    // fluidsynth's internal synth
    fluid_seq_id_t synthId;

    // callback ID for our parent MidiWrapper instance
    fluid_seq_id_t midiwrapperID;

    // callback ID for FluidSeqNoteCallback
    fluid_seq_id_t myselfID;

    // callback ID for N64CSeqWrapper::LoopCallback
    fluid_seq_id_t cseqID = -1;

    fluid_seq_id_t sysexID;

    int defaultSf = -1;
    int defaultBank = -1;
    int defaultProg = -1;

    // fluidsynth's synth has no samplerate getter, so cache it here
    unsigned int cachedSampleRate = 0;

    // tick count when the song ends
    unsigned int lastTick = 0;

    int cachedSf2Id = -1;

    bool lastRenderNotesWithoutPreset;

    // temporary sample mixdown buffer used by fluid_synth_process
    std::vector<float> sampleBuffer;

    // pointer to small buffers within sampleBuffer of dry and effects audio
    std::vector<float*> dry, fx;

    std::vector<bool> midiChannelHasNoteOn;
    std::vector<bool> midiChannelHasProgram;

    std::array<std::array<std::deque<int>, 128>, NMidiChannels> noteOnQueue{};

    void setupSettings();
    void setupMixdownBuffer();
    void setupSynth(const Nullable<string>&);
    void setupSeq(N64CSeqWrapper* cseq);

    void deleteEvents();
    void deleteSynth();
    void deleteSeq();

    void ScheduleNote(const MidiNoteInfo &noteInfo, unsigned int time);
    void NoteOnOff(fluid_event_t *e);

    static void FluidSeqLoopCallback(unsigned int time, fluid_event_t* e, fluid_sequencer_t* seq, void* data);
    static void FluidSeqNoteCallback(unsigned int time, fluid_event_t *e, fluid_sequencer_t *seq, void *data);
    static void FluidSeqSysExCallback(unsigned int time, fluid_event_t *e, fluid_sequencer_t *seq, void *data);
};
