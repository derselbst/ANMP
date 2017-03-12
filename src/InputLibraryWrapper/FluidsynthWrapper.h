
#pragma once

#include "MidiWrapper.h"

#include <fluidsynth.h>

// libsmf - helper lib for reading midi files
#include <smf.h>

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
    FluidsynthWrapper(FluidsynthWrapper const&) = delete;
    FluidsynthWrapper& operator=(FluidsynthWrapper const&) = delete;
    
    static FluidsynthWrapper& Singleton();
    static void seqCallback(unsigned int time, fluid_event_t* e, fluid_sequencer_t* seq, void* data);

    
    void Init(MidiWrapper&);
    // returns the samplerate that will be synthesized at
    int  GetSampleRate();
    // returns the number of audio channels, that will be rendered to
    int  GetChannels();

    void AddEvent(smf_event_t* event, double offset=0.0);
    void ScheduleLoop(MidiLoopInfo* info, size_t);
    void FinishSong(int millisec);
    
    void ReloadConfig();

    void Render(float* bufferToFill, frame_t framesToRender);

private:
    fluid_settings_t* settings = nullptr;
    fluid_synth_t* synth = nullptr;
    fluid_sequencer_t* sequencer = nullptr;
    
    fluid_event_t* synthEvent = nullptr;
    fluid_event_t* callbackEvent = nullptr;

    short synthId;
    Nullable<short> myselfID;

    void setupSettings();
    void setupSynth(MidiWrapper&);
    void setupSeq(MidiWrapper&);

};
