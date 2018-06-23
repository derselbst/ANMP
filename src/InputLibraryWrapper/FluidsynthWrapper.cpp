#include "FluidsynthWrapper.h"
#include "MidiWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Config.h"

#include <chrono>
#include <thread> // std::this_thread::sleep_for
#include <algorithm>

FluidsynthWrapper::FluidsynthWrapper()
{
    if((this->synthEvent = new_fluid_event()) == nullptr ||
       (this->callbackEvent = new_fluid_event()) == nullptr ||
       (this->callbackNoteEvent = new_fluid_event()) == nullptr)
    {
        this->deleteEvents();
        throw std::bad_alloc();
    }
    
    fluid_event_set_source(this->synthEvent, -1);
    fluid_event_set_source(this->callbackEvent, -1);
    fluid_event_set_source(this->callbackNoteEvent, -1);
}

FluidsynthWrapper::~FluidsynthWrapper()
{
    this->deleteSeq();
    this->deleteSynth();
    this->deleteEvents();
    
    delete_fluid_settings(this->settings);
    this->settings = nullptr;
}

void FluidsynthWrapper::deleteEvents()
{
    delete_fluid_event(this->callbackEvent);
    this->callbackEvent = nullptr;
    
    delete_fluid_event(this->callbackNoteEvent);
    this->callbackNoteEvent = nullptr;
    
    delete_fluid_event(this->synthEvent);
    this->synthEvent = nullptr;
}

constexpr int FluidsynthWrapper::GetChannelsPerVoice()
{
    return 2; //stereo
}

void FluidsynthWrapper::setupSeq()
{
    if (this->sequencer == nullptr) // only create the sequencer once
    {
        this->sequencer = new_fluid_sequencer2(false /*i.e. use sample timer*/);
        if (this->sequencer == nullptr)
        {
            THROW_RUNTIME_ERROR("Failed to create the sequencer");
        }

        // register synth as first destination
        this->synthId = fluid_sequencer_register_fluidsynth(this->sequencer, this->synth);
        
        fluid_event_set_dest(this->synthEvent, this->synthId);
    }

    if (this->myselfID.hasValue)
    {
        CLOG(LogLevel_t::Error, "myselfid had a value!");
        // unregister any client
        fluid_sequencer_unregister_client(this->sequencer, this->myselfID.Value);
    }
    // register myself as second destination
    this->myselfID = fluid_sequencer_register_client(this->sequencer, "schedule_note_callback", &FluidsynthWrapper::FluidSeqNoteCallback, this);
    
    // destination may have changed, refresh it
    fluid_event_set_dest(this->callbackNoteEvent, this->myselfID.Value);

    // remove all events from the sequencer's queue
    fluid_sequencer_remove_events(this->sequencer, -1, -1, -1);
    
    this->initTick = fluid_sequencer_get_tick(this->sequencer);
}

void FluidsynthWrapper::deleteSeq()
{
    if (this->sequencer != nullptr)
    {
        // explictly unregister all clients before deleting the seq
        fluid_sequencer_unregister_client(this->sequencer, this->myselfID.Value);
        fluid_event_unregistering(this->synthEvent);
        fluid_sequencer_send_now(this->sequencer, this->synthEvent);
        
        delete_fluid_sequencer(this->sequencer);
        this->sequencer = nullptr;
    }
}

void FluidsynthWrapper::setupSynth(MidiWrapper &midi)
{
    // find a soundfont
    Nullable<string> soundfont;
    if (!gConfig.FluidsynthForceDefaultSoundfont)
    {
        soundfont = ::findSoundfont(midi.Filename);
    }

    if (!soundfont.hasValue)
    {
        // so, either we were forced to use default, or we didnt find any suitable sf2
        soundfont = gConfig.FluidsynthDefaultSoundfont;
    }

    if (!::myExists(soundfont.Value))
    {
        THROW_RUNTIME_ERROR("Cant synthesize this MIDI, soundfont not found: \"" << soundfont.Value << "\"");
    }

    this->cachedSf2 = soundfont.Value;


    if (this->synth == nullptr)
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
    
    if(!gConfig.FluidsynthChannel9IsDrum)
    {
        fluid_synth_set_channel_type(this->synth, 9, CHANNEL_TYPE_MELODIC);
        fluid_synth_bank_select(this->synth, 9, 0); // try to force drum channel to bank 0
    }

    // set highest resampler quality on all channels
    fluid_synth_set_interp_method(this->synth, -1, FLUID_INTERP_HIGHEST);

    // then update samplerate
    fluid_synth_set_sample_rate(this->synth, gConfig.FluidsynthSampleRate);
    this->cachedSampleRate = gConfig.FluidsynthSampleRate;

    constexpr int ACTUAL_FILTERFC_THRESHOLD = 11700 /* Hz */;

    constexpr int CBFD_FILTERFC_CC = 34;
    constexpr int CBFD_FILTERQ_CC = 33;

    // make sure lsb mod and breath controller used by CBFD's IIR lowpass filter are inited to their default value to avoid unhearable instruments
    for (int i = 0; i < fluid_synth_count_midi_channels(this->synth); i++)
    {
        fluid_synth_cc(this->synth, i, CBFD_FILTERQ_CC, 0);
        fluid_synth_cc(this->synth, i, CBFD_FILTERFC_CC, 127);
    }

    fluid_synth_set_custom_filter(this->synth, FLUID_IIR_LOWPASS, FLUID_IIR_NO_GAIN_AMP | FLUID_IIR_Q_LINEAR | FLUID_IIR_Q_ZERO_OFF);

    fluid_mod_t *my_mod = new_fluid_mod();

    // remove default velocity to filter cutoff modulator
    {
        fluid_mod_set_source1(my_mod, FLUID_MOD_VELOCITY,
                              FLUID_MOD_GC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
        fluid_mod_set_source2(my_mod, FLUID_MOD_VELOCITY,
                              FLUID_MOD_GC | FLUID_MOD_SWITCH | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_FILTERFC);
        fluid_synth_remove_default_mod(this->synth, my_mod);
    }

    // add a custom default modulator for CBFD's and JFG's IIR lowpass filter.
    {
        fluid_mod_set_source1(my_mod, CBFD_FILTERFC_CC,
                              FLUID_MOD_CC | FLUID_MOD_SIN | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_source2(my_mod, 0, 0);
        fluid_mod_set_dest(my_mod, GEN_CUSTOM_FILTERFC);
        fluid_mod_set_amount(my_mod, ACTUAL_FILTERFC_THRESHOLD);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }

    // add a custom default modulator Custom CC33 to CBFD's lowpass Filter Q*/
    {
        fluid_mod_set_source1(my_mod, CBFD_FILTERQ_CC,
                              FLUID_MOD_CC | FLUID_MOD_CONCAVE | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_source2(my_mod, 0, 0);
        fluid_mod_set_dest(my_mod, GEN_CUSTOM_FILTERQ);
        fluid_mod_set_amount(my_mod, 7);
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

    // remove default MIDI continuous controller 11 (expression) to initial attenuation mod amount (to make Dinosaur Planet work)
    {
        fluid_mod_set_source1(my_mod,
                              11,
                              FLUID_MOD_CC | FLUID_MOD_CONCAVE | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
        fluid_synth_remove_default_mod(this->synth, my_mod);
    }

    delete_fluid_mod(my_mod);
}

void FluidsynthWrapper::deleteSynth()
{
    if (this->synth != nullptr)
    {
        delete_fluid_synth(this->synth);
        this->synth = nullptr;
    }
}

void FluidsynthWrapper::setupSettings()
{
    if (this->settings == nullptr) // do a full init once
    {
        // deactivate all audio drivers in fluidsynth; we are going to handle audio by ourself, so we dont need this
        const char *DRV[] = {NULL};
        fluid_audio_driver_register(DRV);

        this->settings = new_fluid_settings();
        if (this->settings == nullptr)
        {
            THROW_RUNTIME_ERROR("Failed to create the settings");
        }

        fluid_settings_setint(this->settings, "synth.min-note-length", 1);
        // only in mma mode, bank high and bank low controllers are handled as specified by MIDI standard
        fluid_settings_setstr(this->settings, "synth.midi-bank-select", gConfig.FluidsynthBankSelect.c_str());
        fluid_settings_setint(this->settings, "synth.threadsafe-api", 1);
        fluid_settings_setint(this->settings, "synth.lock-memory", 0);
        fluid_settings_setint(this->settings, "synth.dynamic-sample-loading", 1);
        fluid_settings_setint(this->settings, "synth.polyphony", 2048);
    }

    int stereoChannels = gConfig.FluidsynthMultiChannel ? NMidiChannels : 1;
    fluid_settings_setint(this->settings, "synth.audio-groups", stereoChannels);
    fluid_settings_setint(this->settings, "synth.audio-channels", stereoChannels);
    
    // reverb and chrous settings might have changed
    // those are realtime settings, so they will update the synth in every case
    fluid_settings_setint(this->settings, "synth.chorus.active", gConfig.FluidsynthEnableChorus);
    fluid_settings_setint(this->settings, "synth.reverb.active", gConfig.FluidsynthEnableReverb);
    fluid_settings_setnum(this->settings, "synth.reverb.room-size", gConfig.FluidsynthRoomSize);
    fluid_settings_setnum(this->settings, "synth.reverb.damping", gConfig.FluidsynthDamping);
    fluid_settings_setnum(this->settings, "synth.reverb.width", gConfig.FluidsynthWidth);
    fluid_settings_setnum(this->settings, "synth.reverb.level", gConfig.FluidsynthLevel);
    
    fluid_settings_setint(this->settings, "synth.cpu-cores", 4);
}

void FluidsynthWrapper::setupMixdownBuffer()
{
    constexpr int ChanPerV = FluidsynthWrapper::GetChannelsPerVoice();

    // lookup number of audio and effect (stereo-)channels of the synth
    // see „synth.audio-channels“ and „synth.effects-channels“ settings respectively
    int audVoices = this->GetAudioVoices();
    int fxVoices = this->GetEffectVoices();
    int channels = (audVoices + fxVoices) * ChanPerV;
    
    // allocate one single sample buffer
    this->sampleBuffer.resize(gConfig.FramesToRender * channels);
    
    // array of buffers used to setup channel mapping
    this->dry.resize(audVoices * ChanPerV);
    this->fx.resize(fxVoices * ChanPerV);
    
    // setup buffers to mix dry stereo audio to
    for(int i=0; i<audVoices*ChanPerV; i++)
    {
        this->dry[i] = &this->sampleBuffer.data()[i * gConfig.FramesToRender];
    }
    
    // setup buffers to mix effects stereo audio to
    for(int i=0; i<fxVoices*ChanPerV; i++)
    {
        this->fx[i] = &this->sampleBuffer.data()[audVoices*ChanPerV*gConfig.FramesToRender + i * gConfig.FramesToRender];
    }
}

void FluidsynthWrapper::ShallowInit()
{
    this->setupSettings();
    this->cachedSampleRate = gConfig.FluidsynthSampleRate;
    this->setupMixdownBuffer();
}

void FluidsynthWrapper::DeepInit(MidiWrapper &caller)
{
    this->setupSynth(caller);
    this->setupSeq();
    
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
    if (this->synth != nullptr)
    {
        fluid_synth_sfunload(this->synth, this->cachedSf2Id, true);
    }

    this->deleteSeq();
    // the synth uses pthread_key_create, which quickly runs out of keys when not cleaning up
    this->deleteSynth();
}

void FluidsynthWrapper::ConfigureChannels(SongFormat *f)
{
    int nAudVoices = this->GetAudioVoices();
    int nFxVoices = this->GetEffectVoices();
    int nVoices = nAudVoices + nFxVoices;
    f->SetVoices(nVoices);
    for (int i = 0; i < nVoices; i++)
    {
        f->VoiceChannels[i] = FluidsynthWrapper::GetChannelsPerVoice();
    }

    if (nAudVoices == 1)
    {
        f->VoiceName[0] = "Dry Sound";
    }
    else
    {
        for (int i = 0; i < nAudVoices; i++)
        {
            f->VoiceName[i] = "Midi Channel " + to_string(i);
        }
    }

    int i, fxid = 0;
    fluid_settings_getint(this->settings, "synth.reverb.active", &i);
    if(i)
    {
        f->VoiceName[nAudVoices + fxid] = "Fx Channel " + to_string(fxid) + " (reverb)";
        fxid++;
    }
    
    fluid_settings_getint(this->settings, "synth.chorus.active", &i);
    if(i)
    {
        f->VoiceName[nAudVoices + fxid] = "Fx Channel " + to_string(fxid) + " (chorus)";
        fxid++;
    }
    
//     for (int i = nAudVoices; i < nVoices; i++)
//     {
//         int fxid = i - nAudVoices;
//         f->VoiceName[i] = "Fx Channel " + to_string(fxid);
//         if (fxid == 0)
//         {
//             f->VoiceName[i] += " (reverb)";
//         }
//         else if (fxid == 1)
//         {
//             f->VoiceName[i] += " (chorus)";
//         }
//         else
//         {
//             f->VoiceName[i] += " (unknown fx)";
//         }
//     }
}

int FluidsynthWrapper::GetAudioVoices()
{
    int stereoChannels;
    // dont read from the synth, we need this at a time when the synth hasnt been created
    fluid_settings_getint(this->settings, "synth.audio-channels", &stereoChannels);
    return stereoChannels * 2 / FluidsynthWrapper::GetChannelsPerVoice();
}

int FluidsynthWrapper::GetEffectVoices()
{
    int chan = 0, i;
    
    fluid_settings_getint(this->settings, "synth.reverb.active", &i);
    if(i)
    {
        chan++;
    }
    
    fluid_settings_getint(this->settings, "synth.chorus.active", &i);
    if(i)
    {
        chan++;
    }
    return chan;
//     int chan;
//     // dont read from the synth, we need this at a time when the synth hasnt been created
//     fluid_settings_getint(this->settings, "synth.effects-channels", &chan);
//     return chan * 2 / FluidsynthWrapper::GetChannelsPerVoice();
}

unsigned int FluidsynthWrapper::GetSampleRate()
{
    return this->cachedSampleRate;
}

unsigned int FluidsynthWrapper::GetInitTick()
{
    return this->initTick;
}

void FluidsynthWrapper::AddEvent(smf_event_t *event, double offset)
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
            this->ScheduleNote(n, static_cast<unsigned int>((event->time_seconds + offset) * 1000) + fluid_sequencer_get_tick(this->sequencer));
            return;

        case 0x90:
            n.chan = chan;
            n.key = event->midi_buffer[1];
            n.vel = event->midi_buffer[2];
            this->ScheduleNote(n, static_cast<unsigned int>((event->time_seconds + offset) * 1000) + fluid_sequencer_get_tick(this->sequencer));
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

    ret = fluid_sequencer_send_at(this->sequencer, this->synthEvent, static_cast<unsigned int>((event->time_seconds + offset) * 1000) + fluid_sequencer_get_tick(this->sequencer), true);

    if (ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }
}

// in case MidiWrapper experiences a loop event, schedule an event that calls MidiWrapper back at the end of that loop, so it can feed all the midi events with that loop back to fluidsynth again
void FluidsynthWrapper::ScheduleLoop(MidiLoopInfo *loopInfo)
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
    if (ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }

    }

// use a very complicated way to turn on and off notes by scheduling callbacks to \c this
//
// purpose: using fluid_synth_noteon|off() would kill overlapping notes (i.e. noteons on the same key and channel)
//
// unfortunately many N64 games seem to have overlapping notes in their sequences (or is it only a bug when converting from it to MIDI??)
// anyway, to avoid missing notes we are scheduling a callback on every noteon and off, so that this.FluidSeqNoteCallback() (resp. this.NoteOnOff()) takes care of switching voices on and off
void FluidsynthWrapper::ScheduleNote(const MidiNoteInfo &noteInfo, unsigned int time)
{
    MidiNoteInfo *dup = new MidiNoteInfo(noteInfo);

    fluid_event_timer(this->callbackNoteEvent, dup);

    int ret = fluid_sequencer_send_at(this->sequencer, this->callbackNoteEvent, time, true);
    if (ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }

    }

void FluidsynthWrapper::FluidSeqNoteCallback(unsigned int /*time*/, fluid_event_t *e, fluid_sequencer_t * /*seq*/, void *data)
{
    auto pthis = static_cast<FluidsynthWrapper *>(data);
    auto ninfo = static_cast<MidiNoteInfo *>(fluid_event_get_data(e));
    if (pthis == nullptr || ninfo == nullptr)
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
void FluidsynthWrapper::NoteOnOff(MidiNoteInfo *nInfo)
{
    const int nMidiChan = fluid_synth_count_midi_channels(this->synth);
    const int chan = nInfo->chan;
    const int key = nInfo->key;
    const int vel = nInfo->vel;
    const int activeVoices = fluid_synth_get_active_voice_count(this->synth);
    const bool isNoteOff = vel == 0;

    Nullable<int> id;

    // look through currently playing voices to determine the voice group id to start or stop voices later on
    if (activeVoices > 0)
    {
        vector<fluid_voice_t *> voiceList;
        voiceList.reserve(activeVoices);

        fluid_synth_get_voicelist(this->synth, voiceList.data(), activeVoices, -1);

        fluid_voice_t *v;
        for (int i = 0; i < activeVoices && ((v = voiceList.data()[i]) != nullptr); i++)
        {
            if (fluid_voice_is_on(v) && fluid_voice_get_channel(v) == chan && fluid_voice_get_key(v) == key)
            {
                int foundID = fluid_voice_get_id(v);
                foundID -= key;
                foundID -= chan * 128;
                foundID /= 128 * nMidiChan;

                if (!id.hasValue)
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
                         :
                         max(id.Value, foundID);
                }
            }
        }
    }

    // find a way of creating unique voice group ids depending on channel and key
    //
    // the id needs to include information about what key and what channel the voice is playing on.
    // just do this the same way like accessing a 3 dimensional array
    //     id = (id*128*nMidiChan)+(chan*128)+(key);

    if (isNoteOff)
    {
        if (id.hasValue)
        {
            id = (id.Value * 128 * nMidiChan) + (chan * 128) + (key);
            fluid_synth_stop(this->synth, id.Value);
        }
        else
        {
            // strange, no voice playing at this key and channel was found, however, we got a noteoff for it. silently ignore.
        }
    }
    else
    {
        if (!id.hasValue)
        {
            id = 0;
        }
        else
        {
            id = id.Value + 1;
        }
        id = ((id.Value) * 128 * nMidiChan) + (chan * 128) + (key);
        fluid_preset_t *preset = fluid_synth_get_channel_preset(this->synth, chan);
        if (preset != nullptr)
        {
            fluid_synth_start(this->synth, id.Value, preset, 0, chan, key, vel);
        }
    }
}

void FluidsynthWrapper::FinishSong(int millisec)
{
    for (int i = 0; i < NMidiChannels; i++)
    {
        fluid_event_all_notes_off(this->synthEvent, i);
        fluid_sequencer_send_at(this->sequencer, this->synthEvent, fluid_sequencer_get_tick(this->sequencer) + millisec, true);
    }
}

void FluidsynthWrapper::Render(float *bufferToFill, frame_t framesToRender)
{
    constexpr int ChanPerV = FluidsynthWrapper::GetChannelsPerVoice();

    // lookup number of audio and effect (stereo-)channels of the synth
    // see „synth.audio-channels“ and „synth.effects-channels“ settings respectively
    int audVoices = this->GetAudioVoices();
    int fxVoices = this->GetEffectVoices();
    int channels = (audVoices + fxVoices) * ChanPerV;
    
    float **dry = this->dry.data(), **fx = this->fx.data();
    
    // dont forget to zero sample buffer(s) before each rendering
    std::fill(this->sampleBuffer.begin(), this->sampleBuffer.end(), 0);
    
    int err = fluid_synth_process(this->synth, framesToRender, this->fx.size(), fx, this->dry.size(), dry);
    if(err == FLUID_FAILED)
        THROW_RUNTIME_ERROR("fluid_synth_process() failed!");

    for (int c = 0; c < audVoices; c++)
    {
        for (int frame = 0; frame < framesToRender; frame++)
        {
            // frame by frame write planar audio to interleaved buffer
            bufferToFill[frame * channels + (ChanPerV * c + 0)] = dry[ChanPerV * c + 0][frame];
            bufferToFill[frame * channels + (ChanPerV * c + 1)] = dry[ChanPerV * c + 1][frame];
        }
    }

    for (int c = 0; c < fxVoices; c++)
    {
        for (int frame = 0; frame < framesToRender; frame++)
        {
            int d = audVoices + c;
            bufferToFill[frame * channels + (ChanPerV * d + 0)] = fx[ChanPerV * c + 0][frame];
            bufferToFill[frame * channels + (ChanPerV * d + 1)] = fx[ChanPerV * c + 1][frame];
        }
    }
}
