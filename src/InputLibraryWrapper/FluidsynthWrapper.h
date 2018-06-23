
#pragma once

#include "Nullable.h"
#include "types.h"

#include <fluidsynth.h>

// libsmf - helper lib for reading midi files
#include <smf.h>
#include <vector>

class MidiWrapper;
struct MidiLoopInfo;
struct MidiNoteInfo;
struct SongFormat;

/**
  * class FluidsynthWrapper
  *
  */
class FluidsynthWrapper
{
    public:
    FluidsynthWrapper();
    ~FluidsynthWrapper();

    // forbid copying
    FluidsynthWrapper(FluidsynthWrapper const &) = delete;
    FluidsynthWrapper &operator=(FluidsynthWrapper const &) = delete;

    void ShallowInit();
    void DeepInit(MidiWrapper &);
    void ConfigureChannels(SongFormat *f);
    void Unload();

    // returns the samplerate that will be synthesized at
    unsigned int GetSampleRate();

    int GetAudioVoices();
    int GetEffectVoices();
    static constexpr int GetChannelsPerVoice();

    // returns the tick count the sequencer had during a call to this.DeepInit()
    unsigned int GetInitTick();

    void AddEvent(smf_event_t *event, double offset = 0.0);
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

    // fluidsynth's internal synth
    short synthId;

    // callback ID for ourself
    Nullable<short> myselfID;

    // fluidsynth's synth has no samplerate getter, so cache it here
    unsigned int cachedSampleRate = 0;

    // tick count of the sequencer when this->DeepInit() was called
    unsigned int initTick = 0;

    string cachedSf2;
    int cachedSf2Id = -1;
    
    // temporary sample mixdown buffer used by fluid_synth_process
    std::vector<float> sampleBuffer;
    
    // pointer to small buffers within sampleBuffer of dry and effects audio
    std::vector<float*> dry, fx;

    void setupSettings();
    void setupMixdownBuffer();
    void setupSynth(MidiWrapper &);
    void setupSeq();

    void deleteEvents();
    void deleteSynth();
    void deleteSeq();

    static void FluidSeqNoteCallback(unsigned int time, fluid_event_t *e, fluid_sequencer_t *seq, void *data);
};
