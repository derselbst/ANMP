
#pragma once

#include "MidiWrapper.h"

#include <fluidsynth.h>

// libsmf - helper lib for reading midi files
#include <smf.h>
#include <vector>
#include <memory>

struct MidiNoteInfo;

/**
  * class FluidsynthWrapper
  *
  */
class FluidsynthWrapper
{
    public:
    FluidsynthWrapper(const Nullable<string>& suggestedSf2);
    ~FluidsynthWrapper();

    // forbid copying
    FluidsynthWrapper(FluidsynthWrapper const &) = delete;
    FluidsynthWrapper &operator=(FluidsynthWrapper const &) = delete;

    void ConfigureChannels(SongFormat *f);

    // returns the samplerate that will be synthesized at
    unsigned int GetSampleRate();

    int GetActiveMidiChannels();
    int GetAudioVoices();
    int GetEffectVoices();
    int GetEffectCount();
    static constexpr int GetChannelsPerVoice();

    // returns the tick count the sequencer had during a call to this.DeepInit()
    unsigned int GetInitTick();

    double GetTempoScale(unsigned int uspqn, unsigned int ppqn);

    void AddEvent(smf_event_t *event, double offset = 0.0);
    void ScheduleTempoChange(double newScale, int atTick);
    void ScheduleLoop(MidiLoopInfo *loopInfo);
    void ScheduleNote(const MidiNoteInfo &noteInfo, unsigned int time);
    void NoteOnOff(MidiNoteInfo *nInfo);
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

    // event used for changing the tempo scale of the seq
    fluid_event_t *callbackTempoEvent = nullptr;

    // fluidsynth's internal synth
    fluid_seq_id_t synthId;

    // callback ID for our parent MidiWrapper instance
    fluid_seq_id_t midiwrapperID;

    // callback ID for FluidSeqNoteCallback
    fluid_seq_id_t myselfID;

    // callback ID for FluidSeqTempoCallback
    fluid_seq_id_t myselfTempoID;

    // fluidsynth's synth has no samplerate getter, so cache it here
    unsigned int cachedSampleRate = 0;

    // tick count of the sequencer when this->DeepInit() was called
    unsigned int initTick = 0;
    unsigned int lastTick = 0;

    int cachedSf2Id = -1;

    // temporary sample mixdown buffer used by fluid_synth_process
    std::vector<float> sampleBuffer;

    // pointer to small buffers within sampleBuffer of dry and effects audio
    std::vector<float*> dry, fx;

    // temporary buffer to store all our custom NoteOn events, to make sure they get deleted, even if the NoteOn-callback is not called
    std::vector<std::unique_ptr<MidiNoteInfo>> noteOnContainer;

    std::vector<std::unique_ptr<double>> tempoChangeContainer;

    std::vector<bool> midiChannelHasNoteOn;
    std::vector<bool> midiChannelHasProgram;

    void setupSettings();
    void setupMixdownBuffer();
    void setupSynth();
    void setupSeq();

    void deleteEvents();
    void deleteSynth();
    void deleteSeq();

    static void FluidSeqLoopCallback(unsigned int time, fluid_event_t* e, fluid_sequencer_t* seq, void* data);
    static void FluidSeqNoteCallback(unsigned int time, fluid_event_t *e, fluid_sequencer_t *seq, void *data);
    static void FluidSeqTempoCallback(unsigned int time, fluid_event_t *e, fluid_sequencer_t * seq, void *data);
};
