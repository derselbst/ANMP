#include "FluidsynthWrapper.h"
#include "MidiWrapper.h"
#include "N64CSeqWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Config.h"

#include <chrono>
#include <thread> // std::this_thread::sleep_for
#include <algorithm>


FluidsynthWrapper::FluidsynthWrapper() : lastRenderNotesWithoutPreset(gConfig.FluidsynthRenderNotesWithoutPreset), midiChannelHasNoteOn(NMidiChannels), midiChannelHasProgram(NMidiChannels)
{
    if((this->synthEvent = new_fluid_event()) == nullptr ||
       (this->callbackEvent = new_fluid_event()) == nullptr ||
       (this->callbackNoteEvent = new_fluid_event()) == nullptr ||
       (this->callbackTempoEvent = new_fluid_event()) == nullptr)
    {
        this->deleteEvents();
        throw std::bad_alloc();
    }

    fluid_event_set_source(this->synthEvent, -1);
    fluid_event_set_source(this->callbackEvent, -1);
    fluid_event_set_source(this->callbackNoteEvent, -1);
    fluid_event_set_source(this->callbackTempoEvent, -1);
}

void FluidsynthWrapper::Init(const Nullable<string>& suggestedSf2, N64CSeqWrapper* cseq)
{
    this->setupSettings();
    this->setupSynth(suggestedSf2);
    this->setupSeq(cseq);
}

FluidsynthWrapper::~FluidsynthWrapper()
{
    // soundfonts take up huge amount of memory, free it
    if (this->synth != nullptr)
    {
        fluid_synth_sfunload(this->synth, this->cachedSf2Id, true);
    }

    this->deleteSeq();
    // the synth uses pthread_key_create, which quickly runs out of keys when not cleaning up
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

    delete_fluid_event(this->callbackTempoEvent);
    this->callbackTempoEvent = nullptr;

    delete_fluid_event(this->synthEvent);
    this->synthEvent = nullptr;
}

constexpr int FluidsynthWrapper::GetChannelsPerVoice()
{
    return 2; //stereo
}

void FluidsynthWrapper::setupSeq(N64CSeqWrapper* cseq)
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

    this->midiwrapperID = fluid_sequencer_register_client(this->sequencer, "schedule_loop_callback", &FluidsynthWrapper::FluidSeqLoopCallback, this);
    fluid_event_set_dest(this->callbackEvent, this->midiwrapperID);

    if(cseq != nullptr)
    {
        this->cseqID = fluid_sequencer_register_client(this->sequencer, "schedule_cseq_loop_callback", &N64CSeqWrapper::SequencerCallback, cseq);
    }

    // register myself as second destination
    this->myselfID = fluid_sequencer_register_client(this->sequencer, "schedule_note_callback", &FluidsynthWrapper::FluidSeqNoteCallback, this);
    fluid_event_set_dest(this->callbackNoteEvent, this->myselfID);

    this->myselfTempoID = fluid_sequencer_register_client(this->sequencer, "schedule_tempo_callback", &FluidsynthWrapper::FluidSeqTempoCallback, this);
    fluid_event_set_dest(this->callbackTempoEvent, this->myselfTempoID);

    // remove all events from the sequencer's queue
    fluid_sequencer_remove_events(this->sequencer, -1, -1, -1);
}

void FluidsynthWrapper::SetDefaultSeqTempoScale(unsigned int ppqn)
{
    // initialize to default MIDI tempo
    fluid_sequencer_set_time_scale(this->sequencer, GetTempoScale(500000 /* 120 BPM as per MIDI spec*/, ppqn));
}

void FluidsynthWrapper::deleteSeq()
{
    if (this->sequencer != nullptr)
    {
        // explictly unregister all clients before deleting the seq
        fluid_sequencer_unregister_client(this->sequencer, this->myselfID);
        fluid_sequencer_unregister_client(this->sequencer, this->myselfTempoID);
        fluid_sequencer_unregister_client(this->sequencer, this->midiwrapperID);
        if(this->cseqID != -1)
        {
            fluid_sequencer_unregister_client(this->sequencer, this->cseqID);
        }
        fluid_event_unregistering(this->synthEvent);
        fluid_sequencer_send_now(this->sequencer, this->synthEvent);

        delete_fluid_sequencer(this->sequencer);
        this->sequencer = nullptr;
    }
}

void FluidsynthWrapper::setupSynth(const Nullable<string>& suggestedSf2)
{
    if (this->synth == nullptr)
    {
        /* Create the synthesizer */
        this->synth = new_fluid_synth(this->settings);
        if (this->synth == nullptr)
        {
            THROW_RUNTIME_ERROR("Failed to create the synth");
        }
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

    // find a soundfont
    Nullable<string> soundfont;
    if (!gConfig.FluidsynthForceDefaultSoundfont)
    {
        soundfont = suggestedSf2;
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

    // load the soundfont and assign default channel presets
    if ((this->cachedSf2Id = fluid_synth_sfload(this->synth, soundfont.Value.c_str(), true)) == -1)
    {
        THROW_RUNTIME_ERROR("Specified soundfont seems to be invalid or not supported: \"" << soundfont.Value << "\"");
    }

    constexpr int ACTUAL_FILTERFC_THRESHOLD = 22050/2 /* Hz */;

    constexpr int CBFD_FILTERFC_CC = 34;
    constexpr int CBFD_FILTERQ_CC = 33;
    constexpr int DP_ATTACK_CC = 20;
    constexpr int DP_HOLD_CC = 21;
    constexpr int DP_DECAY_CC = 22;
    constexpr int DP_SUSTAIN_CC = 23;
    constexpr int DP_RELEASE_CC = 24;

    // make sure lsb mod and breath controller used by CBFD's IIR lowpass filter are inited to their default value to avoid unhearable instruments
    for (int i = 0; i < fluid_synth_count_midi_channels(this->synth); i++)
    {
        fluid_synth_cc(this->synth, i, CBFD_FILTERQ_CC, 0);
        fluid_synth_cc(this->synth, i, CBFD_FILTERFC_CC, 127);

        fluid_synth_cc(this->synth, i, DP_ATTACK_CC, 0);
        fluid_synth_cc(this->synth, i, DP_HOLD_CC, 0);
        fluid_synth_cc(this->synth, i, DP_DECAY_CC, 0);
        fluid_synth_cc(this->synth, i, DP_SUSTAIN_CC, 127);
        fluid_synth_cc(this->synth, i, DP_RELEASE_CC, 0);

        if(this->defaultProg == -1)
        {
            int dummy;
            fluid_synth_get_program(this->synth, i, &dummy, &dummy, &this->defaultProg);
        }

        // After startup, the MIDI channels have assigned their default bank and program. We don't want this. When a MIDI channel has no
        // program assigned, no notes shall be played. At least, this is the behaviour found in Jet Force Gemini's software synthesizer.
        // Take a look at the eigth MIDI channel in Sparse 0x25 (Cerulean). Seems like these notes are supposed to be soundeffects. But
        // since the channel is never assigned with a program, the notes are never being heard.
        //
        // That's why, when multichannel rendering is enabled, mute that channel, otherwise don't render that channel at all, by unassigning
        // all default presets here.
        fluid_synth_unset_program(this->synth, i);
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
        fluid_mod_set_source2(my_mod, FLUID_MOD_NONE, 0);
        fluid_mod_set_dest(my_mod, GEN_CUSTOM_FILTERFC);
        fluid_mod_set_amount(my_mod, ACTUAL_FILTERFC_THRESHOLD);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }

    // add a custom default modulator Custom CC33 to CBFD's lowpass Filter Q*/
    {
        fluid_mod_set_source1(my_mod, CBFD_FILTERQ_CC,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_source2(my_mod, FLUID_MOD_NONE, 0);
        fluid_mod_set_dest(my_mod, GEN_CUSTOM_FILTERQ);
        fluid_mod_set_amount(my_mod, gConfig.FluidsynthFilterQ);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }

    fluid_mod_set_source2(my_mod, FLUID_MOD_NONE, 0);
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
    
    // Dinosaur Planet and Star Fox Adventures specific stuff
    fluid_mod_set_source2(my_mod, FLUID_MOD_NONE, 0);
    fluid_mod_set_amount(my_mod, 25000);
    {
        fluid_mod_set_source1(my_mod,
                              DP_ATTACK_CC,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVATTACK);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }
    
    {
        fluid_mod_set_source1(my_mod,
                              DP_HOLD_CC,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVHOLD);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }
    
    {
        fluid_mod_set_source1(my_mod,
                              DP_DECAY_CC,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVDECAY);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }
    
    {
        fluid_mod_set_source1(my_mod,
                              DP_RELEASE_CC,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVRELEASE);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }
    
    {
        fluid_mod_set_source1(my_mod,
                              DP_SUSTAIN_CC,
                              FLUID_MOD_CC | FLUID_MOD_CONVEX | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVSUSTAIN);
        fluid_mod_set_amount(my_mod, 60/*cB*/);
        fluid_synth_add_default_mod(this->synth, my_mod, FLUID_SYNTH_OVERWRITE);
    }

    delete_fluid_mod(my_mod);
}

void FluidsynthWrapper::deleteSynth()
{
    delete_fluid_synth(this->synth);
    this->synth = nullptr;
}

void FluidsynthWrapper::setupSettings()
{
    if (this->settings == nullptr) // do a full init once
    {
        // deactivate all audio drivers in fluidsynth; we are going to handle audio by ourself, so we dont need this
        static const char *DRV[] = {NULL};
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
        fluid_settings_setint(this->settings, "synth.dynamic-sample-loading", 0);
        fluid_settings_setint(this->settings, "synth.polyphony", 2048);
        fluid_settings_setint(this->settings, "synth.cpu-cores", 4);
        // disable high prio threads, you won't have permission anyway
        fluid_settings_setint(this->settings, "audio.realtime-prio", 0);
    }

    fluid_settings_setint(this->settings, "synth.midi-channels", NMidiChannels);
    int stereoChannels = gConfig.FluidsynthMultiChannel ? NMidiChannels : 1;
    fluid_settings_setint(this->settings, "synth.audio-groups", stereoChannels);
    fluid_settings_setint(this->settings, "synth.audio-channels", stereoChannels);
    fluid_settings_setint(this->settings, "synth.effects-groups", stereoChannels);

    // reverb and chrous settings might have changed
    // those are realtime settings, so they will update the synth in every case
    fluid_settings_setint(this->settings, "synth.chorus.active", gConfig.FluidsynthEnableChorus);
    fluid_settings_setint(this->settings, "synth.reverb.active", gConfig.FluidsynthEnableReverb);
    fluid_settings_setnum(this->settings, "synth.reverb.room-size", gConfig.FluidsynthRoomSize);
    fluid_settings_setnum(this->settings, "synth.reverb.damp", gConfig.FluidsynthDamping);
    fluid_settings_setnum(this->settings, "synth.reverb.width", gConfig.FluidsynthWidth);
    fluid_settings_setnum(this->settings, "synth.reverb.level", gConfig.FluidsynthLevel);

    this->cachedSampleRate = gConfig.FluidsynthSampleRate;
    fluid_settings_setnum(this->settings, "synth.sample-rate", this->cachedSampleRate);
}

void FluidsynthWrapper::setupMixdownBuffer()
{
    constexpr int ChanPerV = FluidsynthWrapper::GetChannelsPerVoice();

    // lookup number of audio and effect (stereo-)channels of the synth
    // see „synth.audio-channels“ and „synth.effects-channels“ settings respectively
    int audVoices = this->GetAudioVoices();
    int fxVoices = this->GetEffectVoices();
    int fxCount = this->GetEffectCount();
    int channels = audVoices * ChanPerV;
    
    // allocate one single sample buffer
    this->sampleBuffer.resize(gConfig.FramesToRender * channels);
    
    // array of buffers used to setup channel mapping
    this->dry.resize(channels);
    this->fx.resize(fxVoices * ChanPerV * fxCount);
    
    // setup buffers to mix dry stereo audio to
    for(int i=0; i<channels; i++)
    {
        // if the corresponding MIDI channel has sound, assign a pointer to sample buffer, otherwise assign null so that this audio channel is not rendered
        this->dry[i] = (audVoices==1 || this->midiChannelHasNoteOn[i/ChanPerV]) ? &this->sampleBuffer.data()[i * gConfig.FramesToRender] : nullptr;
    }
    
    for(int i=0; i<fxVoices; i++)
    {
        for(int c=0; c<ChanPerV; c++)
        {
            auto buf = this->dry[(i * ChanPerV + c) % channels];
            
            // mix reverb and chorus to same buffer
            for(int f=0; f<fxCount; f++)
            {
                this->fx[i * fxCount * ChanPerV + f * ChanPerV + c] = buf;
            }
        }
    }
}

void FluidsynthWrapper::ConfigureChannels(SongFormat *f)
{
    this->setupMixdownBuffer();

    int activeMidiChannels = this->GetActiveMidiChannels();
    int nAudVoices = std::min(this->GetAudioVoices(), activeMidiChannels);
    f->SetVoices(nAudVoices);

    if(nAudVoices <= 0)
    {
        return;
    }

    if (!gConfig.FluidsynthMultiChannel)
    {
        f->VoiceChannels[0] = FluidsynthWrapper::GetChannelsPerVoice();
        f->VoiceName[0] = "Everything";
        return;
    }

    unsigned int j = 0;
    for (int i = 0; i < nAudVoices; i++)
    {
        f->VoiceChannels[i] = FluidsynthWrapper::GetChannelsPerVoice();

        for (; j < this->midiChannelHasNoteOn.size(); j++)
        {
            if(this->midiChannelHasNoteOn[j])
            {
                f->VoiceName[i] = (this->cseqID == -1 ? "Midi Channel " : "Sequence Track ") + to_string(j);

                if(!midiChannelHasProgram[j])
                {
                    f->VoiceIsMuted[i] = true;
                    f->VoiceName[i] += " (no program assigned)";
                }
                j++;
                break;
            }
        }
    }
}

int FluidsynthWrapper::GetActiveMidiChannels()
{
    int activeMidiChannels = 0;
    for (unsigned int i = 0; i < this->midiChannelHasNoteOn.size(); i++)
    {
        if(this->midiChannelHasNoteOn[i])
        {
            activeMidiChannels++;
        }
    }
    return activeMidiChannels;
}

int FluidsynthWrapper::GetAudioVoices()
{
    int stereoChannels;
    // dont read from the synth, we need this at a time when the synth hasnt been created
    fluid_settings_getint(this->settings, "synth.audio-channels", &stereoChannels);
    return stereoChannels;
}

int FluidsynthWrapper::GetEffectVoices()
{
    int i;
    fluid_settings_getint(this->settings, "synth.effects-groups", &i);

    return i;
}

int FluidsynthWrapper::GetEffectCount()
{
    int chan;

    // dont read from the synth, we need this at a time when the synth hasnt been created
    fluid_settings_getint(this->settings, "synth.effects-channels", &chan);
    return chan;
}

unsigned int FluidsynthWrapper::GetSampleRate()
{
    return this->cachedSampleRate;
}

double FluidsynthWrapper::GetTempoScale(unsigned int uspqn, unsigned int ppqn)
{
    return (ppqn * 1000.0) // 1000 because the scale is divided by 1000 in fluid_sequencer_get_tick()
           /
           (uspqn / 1000.0); // 1000 because we convert from microsec to millisec
}

void FluidsynthWrapper::AddEvent(smf_event_t *event, double offset)
{
    int ret;

    if(event->midi_buffer[0] == 0xFF && event->midi_buffer[1] == 0x51)
    {
        if (event->midi_buffer_length < 6)
        {
            CLOG(LogLevel_t::Warning, "Ignoring truncated MIDI Tempo message.");
            return;
        }

        int uspqn = (event->midi_buffer[3] << 16) + (event->midi_buffer[4] << 8) + event->midi_buffer[5];
        if (uspqn <= 0)
        {
            CLOG(LogLevel_t::Warning, "Ignoring invalid tempo change.");
            return;
        }

        double scale = GetTempoScale(uspqn, event->track->smf->ppqn);
        CLOG(LogLevel_t::Debug, "Tempo: " << uspqn << " usPQN, " << scale << " scale, " << 60000000.0 / uspqn << " BPM, ppqn: " << event->track->smf->ppqn);

        this->ScheduleTempoChange(scale, event->time_pulses + offset);

        return;
    }

    int key, vel, chan = event->midi_buffer[0] & 0x0F;
    fluid_event_t *fluidEvt = this->synthEvent;
    switch (event->midi_buffer[0] & 0xF0)
    {
        case 0x90:
            fluidEvt = this->callbackNoteEvent;
            key = event->midi_buffer[1];
            vel = event->midi_buffer[2];
            if(vel != 0)
            {
                this->InformHasNoteOn(chan);
                fluid_event_noteon(fluidEvt, chan, key, vel);
                CLOG(LogLevel_t::Debug, "NoteOn, channel " << chan << ", key " << key << ", vel " << vel);
                break;
            }
            [[fallthrough]];

        case 0x80: // noteoff
            fluidEvt = this->callbackNoteEvent;
            key = event->midi_buffer[1];
            fluid_event_noteoff(fluidEvt, chan, key);
            CLOG(LogLevel_t::Debug, "NoteOff, channel " << chan << ", key " << key);
            break;

        case 0xA0:
            fluid_event_key_pressure(fluidEvt, chan, event->midi_buffer[1], event->midi_buffer[2]);
            CLOG(LogLevel_t::Debug, "Aftertouch, channel " << chan << ", note " << static_cast<int>(event->midi_buffer[1]) << ", pressure " << static_cast<int>(event->midi_buffer[2]));
            break;

        case 0xB0: // ctrl change
            // just a usual control change
            fluid_event_control_change(fluidEvt, chan, event->midi_buffer[1], event->midi_buffer[2]);

            CLOG(LogLevel_t::Debug, "Controller, channel " << chan << ", controller " << static_cast<int>(event->midi_buffer[1]) << ", value " << static_cast<int>(event->midi_buffer[2]));
            break;

        case 0xC0:
            this->InformHasProgChange(chan);
            fluid_event_program_change(fluidEvt, chan, event->midi_buffer[1]);
            CLOG(LogLevel_t::Debug, "ProgChange, channel " << chan << ", program " << static_cast<int>(event->midi_buffer[1]));
            break;

        case 0xD0:
            fluid_event_channel_pressure(fluidEvt, chan, event->midi_buffer[1]);
            CLOG(LogLevel_t::Debug, "Channel Pressure, channel " << chan << ", pressure " << static_cast<int>(event->midi_buffer[1]));
            break;

        case 0xE0:
        {
            int16_t pitch = event->midi_buffer[2];
            pitch <<= 7;
            pitch |= event->midi_buffer[1];

            fluid_event_pitch_bend(fluidEvt, chan, pitch);
            CLOG(LogLevel_t::Debug, "Pitch Wheel, channel " << chan << ", value " << pitch);
            break;
        }

        default:
            return;
    }

    ret = fluid_sequencer_send_at(this->sequencer, fluidEvt, static_cast<unsigned int>(event->time_pulses + offset), false);

    if (ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }
}

void FluidsynthWrapper::AddEvent(fluid_event_t *event, uint32_t tick)
{
    if (tick + fluid_sequencer_get_tick(this->sequencer) >= this->lastTick)
    {
        return;
    }

    switch (fluid_event_get_type(event))
    {
        case FLUID_SEQ_NOTE:
        case FLUID_SEQ_NOTEON:
        case FLUID_SEQ_NOTEOFF:
            fluid_event_set_dest(event, fluid_event_get_dest(this->callbackNoteEvent));
            break;

        case FLUID_SEQ_TIMER:
            fluid_event_set_dest(event, cseqID);
            break;

        default:
            fluid_event_set_dest(event, fluid_event_get_dest(this->synthEvent));
            break;
    }

    int ret = fluid_sequencer_send_at(this->sequencer, event, tick, false);
    if (ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }
}

void FluidsynthWrapper::InformHasNoteOn(int chan)
{
    this->midiChannelHasNoteOn[chan] = true;
}

void FluidsynthWrapper::InformHasProgChange(int chan)
{
    this->midiChannelHasProgram[chan] = true;
}


void FluidsynthWrapper::ScheduleTempoChange(double newScale, int atTick)
{
    double* dup = new double(newScale);
    fluid_event_timer(this->callbackTempoEvent, dup);

    int ret = fluid_sequencer_send_at(this->sequencer, this->callbackTempoEvent, atTick, false);
    if (ret != FLUID_OK)
    {
        delete dup;
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
        return;
    }

    this->tempoChangeContainer.emplace_back(dup);
}


void FluidsynthWrapper::FluidSeqTempoCallback(unsigned int /*time*/, fluid_event_t *e, fluid_sequencer_t * seq, void */*data*/)
{
    auto newScale = static_cast<double *>(fluid_event_get_data(e));
    if (newScale == nullptr || fluid_event_get_type(e) != FLUID_SEQ_TIMER)
    {
        return;
    }

    fluid_sequencer_set_time_scale(seq, *newScale);
}

// in case MidiWrapper experiences a loop event, schedule an event that calls MidiWrapper back at the end of that loop, so it can feed all the midi events with that loop back to fluidsynth again
void FluidsynthWrapper::ScheduleLoop(MidiLoopInfo *loopInfo)
{
    int ret;
    unsigned int callbackdate = static_cast<unsigned int>(loopInfo->stop_tick.Value - loopInfo->start_tick.Value); // postpone the callback date by the duration of this midi track loop

    fluid_event_timer(this->callbackEvent, loopInfo);
    ret = fluid_sequencer_send_at(this->sequencer, this->callbackEvent, callbackdate, false);
    if (ret != FLUID_OK)
    {
        CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
    }
}

void FluidsynthWrapper::FluidSeqLoopCallback(unsigned int time, fluid_event_t* e, fluid_sequencer_t* seq, void* data)
{
    (void)seq;

    FluidsynthWrapper* pthis = static_cast<FluidsynthWrapper*>(data);
    MidiLoopInfo* loopInfo = static_cast<MidiLoopInfo*>(fluid_event_get_data(e));

    if(pthis==nullptr || loopInfo == nullptr || fluid_event_get_type(e) != FLUID_SEQ_TIMER)
    {
        return;
    }

    // read in every single event of the loop
    for(unsigned int k=0; k < loopInfo->eventsInLoop.size(); k++)
    {
        smf_event_t* event = loopInfo->eventsInLoop[k];

        // events shall not be looped beyond the end of the song
        if(time + event->time_pulses < pthis->lastTick)
        {
            pthis->AddEvent(event);
        }
    }

    bool isInfinite = loopInfo->count == 0;
    // is there still loop count left? 2 because one loop was already scheduled by parseEvents() and 0 is excluded
    if(isInfinite || loopInfo->count-- > 2)
    {
        pthis->ScheduleLoop(loopInfo);
    }
}

void FluidsynthWrapper::FluidSeqNoteCallback(unsigned int /*time*/, fluid_event_t *e, fluid_sequencer_t * /*seq*/, void *data)
{
    auto pthis = static_cast<FluidsynthWrapper *>(data);
    if (pthis == nullptr)
    {
        return;
    }

    pthis->NoteOnOff(e);
}

static unsigned int computeVoiceId(int chan, int key, int slot)
{
    return slot * 128 * NMidiChannels + 128 * chan + key;
}

// use a very complicated way to turn on and off notes by scheduling callbacks to \c this
//
// purpose: using fluid_synth_noteon|off() would kill overlapping notes (i.e. noteons on the same key and channel)
//
// unfortunately many N64 games seem to have overlapping notes in their sequences (or is it only a bug when converting from it to MIDI??)
// anyway, to avoid missing notes we are scheduling a callback on every noteon and off, so that this.FluidSeqNoteCallback() (resp. this.NoteOnOff()) takes care
// of switching voices on and off via FIFO occurrence of noteon/offs
//
// i.e. the first noteon is switched off by the first noteoff,
//      the second noteon is switched off by the second noteoff, etc.
void FluidsynthWrapper::NoteOnOff(fluid_event_t *e)
{
    const int chan = fluid_event_get_channel(e);
    const int key = fluid_event_get_key(e);
    auto &que = this->noteOnQueue[chan][key];
    unsigned slot;
    unsigned id;

    if (fluid_event_get_type(e) == FLUID_SEQ_PROGRAMSELECT)
    {
        que = this->noteOnQueue[chan][fluid_event_get_bank(e)];
        id = fluid_event_get_sfont_id(e);
        fluid_synth_stop(this->synth, id);

        auto it = std::find(que.begin(), que.end(), id);
        if (it != que.end())
        {
            que.erase(it);
        }
    }
    else if (fluid_event_get_type(e) == FLUID_SEQ_NOTEOFF)
    {
        if(!que.empty())
        {
            slot = que.front();
            id = computeVoiceId(chan, key, slot);
            fluid_synth_stop(this->synth, id);
            que.pop_front();
        }
    }
    else if (fluid_event_get_type(e) == FLUID_SEQ_NOTEON || fluid_event_get_type(e) == FLUID_SEQ_NOTE)
    {
        if(que.empty())
        {
            slot = 0;
        }
        else
        {
            auto it = std::max_element(que.begin(), que.end());
            slot = *it + 1;
        }
        que.push_back(slot);
        id = computeVoiceId(chan, key, slot);

        if (fluid_event_get_type(e) == FLUID_SEQ_NOTE)
        {
            const unsigned int dur = fluid_event_get_duration(e);

            // enqueue the corresponding noteoff event by abusing a progSelect event
            fluid_event_program_select(this->callbackNoteEvent, chan, id, key, 0);

            int ret = fluid_sequencer_send_at(this->sequencer, this->callbackNoteEvent, dur, false);
            if (ret != FLUID_OK)
            {
                CLOG(LogLevel_t::Error, "fluidsynth was unable to queue midi event");
                return;
            }
        }

        // search for a viable preset to turn on this note
        fluid_preset_t *preset = fluid_synth_get_channel_preset(this->synth, chan);
        if (preset == nullptr)
        {
            // this MIDI channel has no preset assigned yet

            int sf, bnk, dummy;
            fluid_synth_get_program(this->synth, chan, &sf, &bnk, &dummy);
            fluid_sfont_t* sfont = fluid_synth_get_sfont(this->synth, sf);
            fluid_preset_t *defaultPreset = fluid_sfont_get_preset(sfont, bnk, this->defaultProg);

            if(this->lastRenderNotesWithoutPreset)
            {
                preset = defaultPreset;
            }
            else // we should not render notes without assigned preset
            {
                if(this->midiChannelHasProgram[chan])
                {
                    // At some time in the future, there will be a program assigned. For now, do nothing, this note remains unsounded. (JetForceGemini Sparse 0x29)
                }
                else
                {
                    // A program will never be assigned to this channel. Play the note with the default preset.
                    // In case FluidsynthMultiChannel is true, we will mute that channel later on.
                    preset = defaultPreset;
                }
            }
        }

        // turn it on finally
        if(preset != nullptr)
        {
            const int vel = fluid_event_get_velocity(e);
            fluid_synth_start(this->synth, id, preset, 0, chan, key, vel);
        }
    }
}

void FluidsynthWrapper::FinishSong(int ticks)
{
    fluid_event_all_notes_off(this->synthEvent, -1);
    this->lastTick = ticks;
    fluid_sequencer_send_at(this->sequencer, this->synthEvent, ticks, false);
}

void FluidsynthWrapper::Render(float *bufferToFill, frame_t framesToRender)
{
    constexpr int ChanPerV = FluidsynthWrapper::GetChannelsPerVoice();

    // lookup number of audio and effect (stereo-)channels of the synth
    // see „synth.audio-channels“ and „synth.effects-channels“ settings respectively
    int audVoices = this->GetAudioVoices();
    int channels = std::min(audVoices, this->GetActiveMidiChannels()) * ChanPerV;
    
    float **dry = this->dry.data(), **fx = this->fx.data();
    
    // dont forget to zero sample buffer(s) before each rendering
    std::fill(this->sampleBuffer.begin(), this->sampleBuffer.end(), 0);
    
    int err = fluid_synth_process(this->synth, framesToRender, this->fx.size(), fx, this->dry.size(), dry);
    if(err == FLUID_FAILED)
        THROW_RUNTIME_ERROR("fluid_synth_process() failed!");

    for (int in = 0, out = 0; in < audVoices; in++)
    {
        if(audVoices == 1 || this->midiChannelHasNoteOn[in])
        {
            for (int frame = 0; frame < framesToRender; frame++)
            {
                // frame by frame write planar audio to interleaved buffer
                bufferToFill[frame * channels + (ChanPerV * out + 0)] = dry[ChanPerV * in + 0][frame];
                bufferToFill[frame * channels + (ChanPerV * out + 1)] = dry[ChanPerV * in + 1][frame];
            }
            out++;
        }
    }
}
