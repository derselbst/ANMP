#include "FluidsynthWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>

FluidsynthWrapper::FluidsynthWrapper()
{}

FluidsynthWrapper::~FluidsynthWrapper ()
{
    if(this->callbackEvent != nullptr)
    {
        delete_fluid_event(this->callbackEvent);
        this->callbackEvent = nullptr;
    }
        
    if(this->synthEvent != nullptr)
    {
        delete_fluid_event(this->synthEvent);
        this->synthEvent = nullptr;
    }
    
    this->deleteSeq();
    this->deleteSynth();
    
    if(this->settings != nullptr)
    {
        delete_fluid_settings(this->settings);
        this->settings = nullptr;
    }
}

void FluidsynthWrapper::setupSeq(MidiWrapper& midi)
{
    if(this->sequencer == nullptr) // only create the sequencer once
    {
        this->sequencer = new_fluid_sequencer2(false /*i.e. use sample timer*/);
        if (this->sequencer == nullptr)
        {
            THROW_RUNTIME_ERROR("Failed to create the sequencer");
        }
    
        // register synth as first destination
        this->synthId = fluid_sequencer_register_fluidsynth(this->sequencer, this->synth);
    }
    
    if(this->myselfID.hasValue)
    {
        // unregister any client
        fluid_sequencer_unregister_client(this->sequencer, this->myselfID.Value);
    }
    // register myself as second destination
    this->myselfID = fluid_sequencer_register_client(this->sequencer, "schedule_loop_callback", &MidiWrapper::FluidSeqCallback, &midi);
    
    // remove all events from the sequencer's queue
    fluid_sequencer_remove_events(this->sequencer, -1, -1, -1);
}

void FluidsynthWrapper::deleteSeq()
{
    if(this->sequencer != nullptr)
    {
        delete_fluid_sequencer(this->sequencer);
        this->sequencer = nullptr;
    }
}

void FluidsynthWrapper::setupSynth(MidiWrapper& midi)
{
    if(this->synth == nullptr)
    {
        /* Create the synthesizer */
        this->synth = new_fluid_synth(this->settings);
        if (this->synth == nullptr)
        {
            THROW_RUNTIME_ERROR("Failed to create the synth");
        }
        // retrieve this after the synth has been inited (just for sure)
        fluid_settings_getint(this->settings, "audio.period-size", &gConfig.FluidsynthPeriodSize);
    }
    
    // press the big red panic/reset button
    fluid_synth_system_reset(this->synth);
    fluid_synth_bank_select(this->synth, 9, 0); // try to force drum channel to bank 0
    
    // reverb and chrous settings might have changed
    fluid_synth_set_reverb_on(this->synth, gConfig.FluidsynthEnableReverb);
    fluid_synth_set_reverb(this->synth, gConfig.FluidsynthRoomSize, gConfig.FluidsynthDamping, gConfig.FluidsynthWidth, gConfig.FluidsynthLevel);
    
    fluid_synth_set_chorus_on(this->synth, gConfig.FluidsynthEnableChorus);
    
    // then update samplerate
    fluid_synth_set_sample_rate(this->synth, gConfig.FluidsynthSampleRate);
    this->cachedSampleRate = gConfig.FluidsynthSampleRate;

    // find a soundfont
    Nullable<string> soundfont;
    if(!gConfig.FluidsynthForceDefaultSoundfont)
    {
        soundfont = ::findSoundfont(midi.Filename);
    }

    if(!soundfont.hasValue)
    {
        // so, either we were forced to use default, or we didnt find any suitable sf2
        soundfont = gConfig.FluidsynthDefaultSoundfont;
    }

    if(!::myExists(soundfont.Value))
    {
        THROW_RUNTIME_ERROR("Cant synthesize this MIDI, soundfont not found: \"" << soundfont.Value << "\"");
    }
    
    this->cachedSf2 = soundfont.Value;
}

void FluidsynthWrapper::deleteSynth()
{
    if(this->synth != nullptr)
    {
        delete_fluid_synth(this->synth);
        this->synth = nullptr;
    }
}

void FluidsynthWrapper::setupSettings()
{
    if(this->settings == nullptr) // do a full init once
    {
        this->settings = new_fluid_settings();
        if (this->settings == nullptr)
        {
            THROW_RUNTIME_ERROR("Failed to create the settings");
        }

        fluid_settings_setint(this->settings, "synth.min-note-length", 1);
        // only in mma mode, bank high and bank low controllers are handled as specified by MIDI standard
        fluid_settings_setstr(this->settings, "synth.midi-bank-select", "mma");

        // these maybe needed for fast renderer (even fluidsynth itself isnt sure about)
        fluid_settings_setstr(this->settings, "player.timing-source", "sample");
        fluid_settings_setint(this->settings, "synth.parallel-render", 0);
        fluid_settings_setint(this->settings, "synth.threadsafe-api", 0);
    }
    
    int stereoChannels = gConfig.FluidsynthMultiChannel ? NMidiChannels : 1;
    fluid_settings_setint(this->settings, "synth.audio-groups",    stereoChannels);
    fluid_settings_setint(this->settings, "synth.audio-channels",  stereoChannels);
}

void FluidsynthWrapper::ShallowInit()
{
    this->setupSettings();
    this->cachedSampleRate = gConfig.FluidsynthSampleRate;
}

void FluidsynthWrapper::DeepInit(MidiWrapper& caller)
{
    this->setupSynth(caller);
    this->setupSeq(caller);
    this->initTick = fluid_sequencer_get_tick(this->sequencer);
    
    if(this->synthEvent == nullptr)
    {
        this->synthEvent = new_fluid_event();
        fluid_event_set_source(this->synthEvent, -1);
        fluid_event_set_dest(this->synthEvent, this->synthId);
    }
    
    if(this->callbackEvent == nullptr)
    {
        this->callbackEvent = new_fluid_event();
        fluid_event_set_source(this->callbackEvent, -1);
    }
    
    /* Load the soundfont */
    if (!fluid_is_soundfont(this->cachedSf2.c_str()))
    {
        THROW_RUNTIME_ERROR("Specified soundfont seems to be invalid (weak test): \"" << this->cachedSf2 << "\"");
    }
    
    if ((this->cachedSf2Id = fluid_synth_sfload(this->synth, this->cachedSf2.c_str(), true)) == -1)
    {
        THROW_RUNTIME_ERROR("Specified soundfont seems to be invalid (strong test): \"" << this->cachedSf2 << "\"");
    }
}

void FluidsynthWrapper::Unload()
{
    // soundfonts take up huge amount of memory, free it
    fluid_synth_sfunload(this->synth, this->cachedSf2Id, true);
    
    this->deleteSeq();
    // the synth uses pthread_key_create, which quickly runs out of keys when not cleaning up
    this->deleteSynth();
}

unsigned int FluidsynthWrapper::GetChannels()
{
    int stereoChannels;
    fluid_settings_getint(this->settings, "synth.audio-channels", &stereoChannels);
    return stereoChannels*2;
}

unsigned int FluidsynthWrapper::GetSampleRate()
{
    return this->cachedSampleRate;
}

unsigned int FluidsynthWrapper::GetInitTick()
{
    return this->initTick;
}

void FluidsynthWrapper::AddEvent(smf_event_t * event, double offset)
{
    int ret; // fluidsynths return value

    uint16_t chan = event->midi_buffer[0] & 0x0F;
    switch (event->midi_buffer[0] & 0xF0)
    {
    case 0x80: // notoff
        fluid_event_noteoff(this->synthEvent, chan, event->midi_buffer[1]);
        break;

    case 0x90:
        fluid_event_noteon(this->synthEvent, chan, event->midi_buffer[1], event->midi_buffer[2]);
        break;

    case 0xA0:
        CLOG(LogLevel_t::Debug, "Aftertouch, channel " << chan << ", note " << static_cast<int>(event->midi_buffer[1]) << ", pressure " << static_cast<int>(event->midi_buffer[2]));
        CLOG(LogLevel_t::Error, "Fluidsynth does not support key specific pressure (aftertouch); discarding event");
        return;
        break;

    case 0xB0: // ctrl change
    {
//         if(IsLoopStart(event) || IsLoopStop(event))
//         {
//             // loops are handled within MidiWrapper, we dont even receive them here
//         }
        // just a usual control change
        fluid_event_control_change(this->synthEvent, chan, event->midi_buffer[1], event->midi_buffer[2]);

        CLOG(LogLevel_t::Debug, "Controller, channel " << chan << ", controller " << static_cast<int>(event->midi_buffer[1]) << ", value " << static_cast<int>(event->midi_buffer[2]));
    }
    break;

    case 0xC0:
        fluid_event_program_change(this->synthEvent, chan, event->midi_buffer[1]);
        CLOG(LogLevel_t::Debug, "ProgChange, channel " << chan << ", program " << static_cast<int>(event->midi_buffer[1]));
        break;

    case 0xD0:
        fluid_event_channel_pressure(this->synthEvent, chan, event->midi_buffer[1]);
        CLOG(LogLevel_t::Debug, "Channel Pressure, channel " << chan << ", pressure " << static_cast<int>(event->midi_buffer[1]));
        break;

    case 0xE0:
    {
        int16_t pitch = event->midi_buffer[2];
        pitch <<= 7;
        pitch |= event->midi_buffer[1];

        fluid_event_pitch_bend(this->synthEvent, chan, pitch);
        CLOG(LogLevel_t::Debug, "Pitch Wheel, channel " << chan << ", value " << pitch);
    }
    break;

    default:
        return;
    }

    ret = fluid_sequencer_send_at(this->sequencer, this->synthEvent, static_cast<unsigned int>((event->time_seconds - offset)*1000) + fluid_sequencer_get_tick(this->sequencer), true);

    if(ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }
}

void FluidsynthWrapper::ScheduleLoop(MidiLoopInfo* loopInfo)
{
    unsigned int callbackdate = fluid_sequencer_get_tick(this->sequencer); // now
    
    callbackdate += static_cast<unsigned int>((loopInfo->stop.Value - loopInfo->start.Value) * 1000); // postpone the callback date by the duration of this midi track loop
    
    fluid_event_timer(this->callbackEvent, loopInfo);

    int ret = fluid_sequencer_send_at(this->sequencer, this->callbackEvent, callbackdate, true);
    if(ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }

    return;
}

void FluidsynthWrapper::FinishSong(int millisec)
{
    for(int i=0; i<NMidiChannels; i++)
    {
        fluid_event_all_notes_off(this->synthEvent, i);
        fluid_sequencer_send_at(this->sequencer, this->synthEvent, fluid_sequencer_get_tick(this->sequencer) + millisec, true);
    }
}
    
void FluidsynthWrapper::Render(float* bufferToFill, frame_t framesToRender)
{
    int channels = this->GetChannels();
    
    // fluid_synth_process renders planar audio, i.e. each midi channel gets render to its own buffer, rather than having one buffer and interleaving PCM
    float** temp_buf = new float*[channels];
    for(int i = 0; i< channels; i++)
    {
        temp_buf[i] = new float[framesToRender];
    }

    fluid_synth_process(this->synth, framesToRender, 0, nullptr, channels, temp_buf);
    for(int frame=0; frame<framesToRender; frame++)
    {
        for(int c=0; c<channels; c++)
        {
            // frame by frame write planar audio to interleaved buffer
            bufferToFill[frame * channels + c] = temp_buf[c][frame];
        }
    }
    
    for(int i=0; i < channels; i++)
    {
        delete[] temp_buf[i];
    }
    delete[] temp_buf;
}
