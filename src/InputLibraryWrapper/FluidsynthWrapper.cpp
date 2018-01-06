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
    
    if(this->callbackNoteEvent != nullptr)
    {
        delete_fluid_event(this->callbackNoteEvent);
        this->callbackNoteEvent = nullptr;
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
    
    if(this->midiwrapperID.hasValue)
    {
        // unregister any client
        fluid_sequencer_unregister_client(this->sequencer, this->midiwrapperID.Value);
    }
    // register myself as second destination
    this->midiwrapperID = fluid_sequencer_register_client(this->sequencer, "schedule_loop_callback", &MidiWrapper::FluidSeqCallback, &midi);
        
    if(this->myselfID.hasValue)
    {
        // unregister any client
        fluid_sequencer_unregister_client(this->sequencer, this->myselfID.Value);
    }
    // register myself as second destination
    this->myselfID = fluid_sequencer_register_client(this->sequencer, "schedule_note_callback", &FluidsynthWrapper::FluidSeqNoteCallback, this);
    
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
    fluid_synth_set_channel_type(this->synth, 9, CHANNEL_TYPE_MELODIC);
    fluid_synth_bank_select(this->synth, 9, 0); // try to force drum channel to bank 0
        
    // increase default polyphone
    fluid_synth_set_polyphony(this->synth, 1024*4);
    
    // set highest resampler quality on all channels
    fluid_synth_set_interp_method(this->synth, -1, FLUID_INTERP_HIGHEST);
    
    // reverb and chrous settings might have changed
    fluid_synth_set_reverb_on(this->synth, gConfig.FluidsynthEnableReverb);
    fluid_synth_set_reverb(this->synth, gConfig.FluidsynthRoomSize, gConfig.FluidsynthDamping, gConfig.FluidsynthWidth, gConfig.FluidsynthLevel);
    
    fluid_synth_set_chorus_on(this->synth, gConfig.FluidsynthEnableChorus);
    
    // make sure lsb mod and breath controller used by CBFD's IIR lowpass filter are inited to their default value to avoid unhearable instruments
    for(int i=0; i<fluid_synth_count_midi_channels(this->synth); i++)
    {
        fluid_synth_cc(this->synth, i, 33, 0);
        fluid_synth_cc(this->synth, i, 34, 127);
    }
    
    // then update samplerate
    fluid_synth_set_sample_rate(this->synth, gConfig.FluidsynthSampleRate);
    this->cachedSampleRate = gConfig.FluidsynthSampleRate;
    
#if FLUIDSYNTH_VERSION_MAJOR >= 2
    fluid_mod_t* my_mod = new_fluid_mod();
    
    // add a default modulator for CBFD's and JFG's IIR lowpass filter.
    {
        fluid_mod_set_source1(my_mod, 34,
                    FLUID_MOD_CC
                    | FLUID_MOD_SIN
                    | FLUID_MOD_UNIPOLAR
                    | FLUID_MOD_POSITIVE
                    );
        fluid_mod_set_source2(my_mod, 0, 0);
        fluid_mod_set_dest(my_mod, GEN_CUSTOM_FILTERFC);
        fluid_mod_set_amount(my_mod, 10000);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }
    
    fluid_mod_set_source2(my_mod, 0, 0);
    fluid_mod_set_dest(my_mod, GEN_ATTENUATION);
    fluid_mod_set_amount(my_mod, 960 * 0.4);
    
    // override default MIDI Note-On Velocity to Initial Attenuation modulator amount
    {
        fluid_mod_set_source1(my_mod,
                                FLUID_MOD_VELOCITY,
                                FLUID_MOD_GC | FLUID_MOD_CONCAVE | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }
    
    // override default MIDI continuous controller 7 (main volume) to initial attenuation mod amount
    {
        fluid_mod_set_source1(my_mod,
                            7,
                            FLUID_MOD_CC | FLUID_MOD_CONCAVE | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }
    
    // override default MIDI continuous controller 11 (expression) to initial attenuation mod amount
    {
        fluid_mod_set_source1(my_mod,
                            11,
                            FLUID_MOD_CC | FLUID_MOD_CONCAVE | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }
    
    delete_fluid_mod(my_mod);
#else
#warning "Cannot simulate Rareware's IIR Lowpass Filter used in CBFD and JFG. Fluidsynth too old, use at least version 2.0"
#endif

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
        fluid_settings_setint(this->settings, "synth.threadsafe-api", 1);
        fluid_settings_setint(this->settings, "synth.lock-memory", 0);
    }
    
    int stereoChannels = gConfig.FluidsynthMultiChannel ? NMidiChannels : 1;
    fluid_settings_setint(this->settings, "synth.audio-groups",    stereoChannels);
    fluid_settings_setint(this->settings, "synth.audio-channels",  stereoChannels);
    
//     fluid_settings_setstr(this->settings, "synth.volenv", "compliant");
    fluid_settings_setstr(this->settings, "synth.cpu-cores", "4");
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
    // destination may have changed, refresh it
    fluid_event_set_dest(this->callbackEvent, this->midiwrapperID.Value);
    
    if(this->callbackNoteEvent == nullptr)
    {
        this->callbackNoteEvent = new_fluid_event();
        fluid_event_set_source(this->callbackNoteEvent, -1);
    }
    // destination may have changed, refresh it
    fluid_event_set_dest(this->callbackNoteEvent, this->myselfID.Value);
    
    // Load the soundfont
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
    if(this->synth != nullptr)
    {
        fluid_synth_sfunload(this->synth, this->cachedSf2Id, true);
    }
    
    this->deleteSeq();
    // the synth uses pthread_key_create, which quickly runs out of keys when not cleaning up
    this->deleteSynth();
}

void FluidsynthWrapper::ConfigureChannels(SongFormat* f)
{
    unsigned int nAudVoices = this->GetAudioVoices();
    unsigned int nFxVoices = this->GetEffectVoices();
    unsigned int nVoices = nAudVoices + nFxVoices;
    f->SetVoices(nVoices);
    for(unsigned int i=0; i<nVoices; i++)
    {
        f->VoiceChannels[i] = this->GetChannelsPerVoice();
    }
    
    if(nAudVoices==1)
    {
        f->VoiceName[0] = "Dry Sound";
    }
    else
    {
        for(unsigned int i=0; i<nAudVoices; i++)
        {
            f->VoiceName[i] = "Midi Channel " + to_string(i);
        }
    }
    
    for(unsigned int i=nAudVoices; i<nVoices; i++)
    {
        unsigned int fxid = i-nAudVoices;
        f->VoiceName[i] = "Fx Channel " + to_string(fxid);
        if(fxid==0)
            f->VoiceName[i] += " (reverb)";
        else if(fxid==1)
            f->VoiceName[i] += " (chorus)";
        else
            f->VoiceName[i] += " (unknown fx)";
    }
    
}

constexpr unsigned int FluidsynthWrapper::GetChannelsPerVoice()
{
    return 2; //stereo
}

unsigned int FluidsynthWrapper::GetAudioVoices()
{
    int stereoChannels;
    fluid_settings_getint(this->settings, "synth.audio-channels", &stereoChannels);
    return stereoChannels*2 / FluidsynthWrapper::GetChannelsPerVoice();
}

unsigned int FluidsynthWrapper::GetEffectVoices()
{
    int chan;
    fluid_settings_getint(this->settings, "synth.effects-channels", &chan);
    return chan*2 / FluidsynthWrapper::GetChannelsPerVoice();
}

unsigned int FluidsynthWrapper::GetVoices()
{
    return this->GetAudioVoices() + this->GetEffectVoices();
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
    MidiNoteInfo n;

    uint16_t chan = event->midi_buffer[0] & 0x0F;
    switch (event->midi_buffer[0] & 0xF0)
    {
    case 0x80: // notoff
        n.chan = chan;
        n.key = event->midi_buffer[1];
        n.vel = 0;
        this->ScheduleNote(n, static_cast<unsigned int>((event->time_seconds - offset)*1000) + fluid_sequencer_get_tick(this->sequencer));
        return;

    case 0x90:
        n.chan = chan;
        n.key = event->midi_buffer[1];
        n.vel = event->midi_buffer[2];
        this->ScheduleNote(n, static_cast<unsigned int>((event->time_seconds - offset)*1000) + fluid_sequencer_get_tick(this->sequencer));
        return;

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

// in case MidiWrapper experiences a loop event, schedule an event that calls MidiWrapper back at the end of that loop, so it can feed all the midi events with that loop back to fluidsynth again
void FluidsynthWrapper::ScheduleLoop(MidiLoopInfo* loopInfo)
{
    int ret;
    unsigned int callbackdate = fluid_sequencer_get_tick(this->sequencer); // now
    
    callbackdate += static_cast<unsigned int>((loopInfo->stop.Value - loopInfo->start.Value) * 1000); // postpone the callback date by the duration of this midi track loop
    
    // HACK: before scheduling the callback, make sure no note is sounding anymore at loopend
    // for some ambiance tunes of DK64 the noteoff events happen after the loopend, as a consequence, note the have been turned on during a note will never be stopped.
    // this hack however breaks JFG, so disable it for now
#if 0
    fluid_event_all_notes_off(this->synthEvent, loopInfo->channel);
    ret = fluid_sequencer_send_at(this->sequencer, this->synthEvent, callbackdate-1, true);
    if(ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }
#endif
    
    fluid_event_timer(this->callbackEvent, loopInfo);
    ret = fluid_sequencer_send_at(this->sequencer, this->callbackEvent, callbackdate, true);
    if(ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }

    return;
}

// use a very complicated way to turn on and off notes by scheduling callbacks to \c this
// 
// purpose: using fluid_synth_noteon|off() would kill overlapping notes (i.e. noteons on the same key and channel)
//
// unfortunately many N64 games seem to have overlapping notes in their sequences (or is it only a bug when converting from it to MIDI??)
// anyway, to avoid missing notes we are scheduling a callback on every noteon and off, so that this.FluidSeqNoteCallback() (resp. this.NoteOnOff()) takes care of switching voices on and off
void FluidsynthWrapper::ScheduleNote(const MidiNoteInfo& noteInfo, unsigned int time)
{
    MidiNoteInfo* dup = new MidiNoteInfo(noteInfo);
     
    fluid_event_timer(this->callbackNoteEvent, dup);

    int ret = fluid_sequencer_send_at(this->sequencer, this->callbackNoteEvent, time, true);
    if(ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }

    return;
}

void FluidsynthWrapper::FluidSeqNoteCallback(unsigned int /*time*/, fluid_event_t* e, fluid_sequencer_t* /*seq*/, void* data)
{
    auto pthis = static_cast<FluidsynthWrapper*>(data);
    auto ninfo = static_cast<MidiNoteInfo*>(fluid_event_get_data(e));
    if(pthis==nullptr || ninfo==nullptr)
    {
        return;
    }
    
    pthis->NoteOnOff(ninfo);
    
    delete ninfo;
}

// switch voices on and off via FIFO occurrence of noteon/offs
//
// i.e. the first noteon is switched off by the first noteoff,
//      the second noteon is switched off by the second noteoff, etc.
void FluidsynthWrapper::NoteOnOff(MidiNoteInfo* nInfo)
{
    const int nMidiChan = fluid_synth_count_midi_channels(this->synth);
    const int chan = nInfo->chan;
    const int key = nInfo->key;
    const int vel = nInfo->vel;
    const int activeVoices = fluid_synth_get_active_voice_count(this->synth);
    const bool isNoteOff = vel == 0;
    
    Nullable<int> id;
    
    // look through currently playing voices to determine the voice group id to start or stop voices later on
    if(activeVoices > 0)
    {
        vector<fluid_voice_t*> voiceList;
        voiceList.reserve(activeVoices);
        
        fluid_synth_get_voicelist(this->synth, voiceList.data(), activeVoices, -1);
        
        fluid_voice_t* v;
        for(int i=0; i<activeVoices && ((v=voiceList.data()[i])!=nullptr); i++)
        {
            if(fluid_voice_is_on(v)
                && fluid_voice_get_channel(v) == chan
                && fluid_voice_get_key(v) == key)
            {
                int foundID = fluid_voice_get_id(v);
                foundID -= key;
                foundID -= chan*128;
                foundID /= 128*nMidiChan;
                
                if(!id.hasValue)
                {
                    // so if this is the first voice already playing at this key and chan found, just use its id
                    id = foundID;
                }
                else
                {
                    // else if this is a noteoff:
                    id = isNoteOff ?
                    // find the id of that voice that was switched on first (smallest id)
                    min(id.Value, foundID)
                    // it's a noteon? find out how many voices of the same key, chan are already playing (get largest id available)
                    : max(id.Value, foundID);
                }
            }
        }
    }
    
    // find a way of creating unique voice group ids depending on channel and key
    //
    // the id needs to include information about what key and what channel the voice is playing on.
    // just do this the same way like accessing a 3 dimensional array
//     id = (id*128*nMidiChan)+(chan*128)+(key);
    
    if(isNoteOff)
    {
        if(id.hasValue)
        {
            id = (id.Value*128*nMidiChan)+(chan*128)+(key);
            fluid_synth_stop(this->synth, id.Value);
        }
        else
        {
            // strange, no voice playing at this key and channel was found, however, we got a noteoff for it. silently ignore.
        }
    }
    else
    {
        if(!id.hasValue)
        {
            id = 0;
        }
        else
        {
            id = id.Value+1;
        }
        id = ((id.Value)*128*nMidiChan)+(chan*128)+(key);
        fluid_preset_t* preset = fluid_synth_get_channel_preset(this->synth, chan);
        if(preset != nullptr)
        {
            fluid_synth_start(this->synth, id.Value, preset, 0, chan, key, vel);
        }
    }
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
    constexpr unsigned int chanPerV = FluidsynthWrapper::GetChannelsPerVoice();
    int channels = this->GetVoices() * chanPerV;
    
    // fluid_synth_process renders planar audio, i.e. each midi channel gets render to its own buffer, rather than having one buffer and interleaving PCM
    float** temp_buf = new float*[channels];
    for(int i = 0; i< channels; i++)
    {
        temp_buf[i] = new float[framesToRender];
        
        // in case fluidsynth doesnt fill up all buffers, zero them out to avoid noise
        memset(temp_buf[i], 0, framesToRender*sizeof(float));
    }
    
    int audVoices = this->GetAudioVoices();
    int fxVoices = this->GetEffectVoices();
    float** mix_buf_l = &temp_buf[0];
    float** mix_buf_r = &mix_buf_l[audVoices];
    
    float** fx_buf_l = &mix_buf_r[audVoices];
    float** fx_buf_r = &fx_buf_l[fxVoices];
    fluid_synth_nwrite_float(this->synth, framesToRender, mix_buf_l, mix_buf_r, fx_buf_l, fx_buf_r);
        
    for(int c=0; c<audVoices; c++)
    {
        for(int frame=0; frame<framesToRender; frame++)
        {
            // frame by frame write planar audio to interleaved buffer
            bufferToFill[frame * channels + (chanPerV*c+0)] = mix_buf_l[c][frame];
            bufferToFill[frame * channels + (chanPerV*c+1)] = mix_buf_r[c][frame];
        }
    }
    
    for(int c=0; c<fxVoices; c++)
    {
        for(int frame=0; frame<framesToRender; frame++)
        {
            int d = audVoices+c;
            bufferToFill[frame * channels + (chanPerV*d+0)] = fx_buf_l[c][frame];
            bufferToFill[frame * channels + (chanPerV*d+1)] = fx_buf_r[c][frame];
        }
    }
    
    for(int i=0; i < channels; i++)
    {
        delete[] temp_buf[i];
    }
    delete[] temp_buf;
}
