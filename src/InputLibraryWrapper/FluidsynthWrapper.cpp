#include "FluidsynthWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#define IsControlChange(e) ((e->midi_buffer[0] & 0xF0) == 0xB0)
#define IsLoopStart(e) (e->midi_buffer[1] == gConfig.MidiControllerLoopStart)
#define IsLoopStop(e) (e->midi_buffer[1] == gConfig.MidiControllerLoopStop)


/** This class got pretty complex. want to see an easier form? Goto git commit d3961aec428adf4eec59c90f57fd93d890cf1499
 *
 * Things we are doing here:
 *
 *  1. use libsmf to retrieve midi events from a midi file
 *  2. feed these event to fluidsynth's sequencer (fluidseq)
 *  3. fluidseq manages an internal queue. on every call to fluid_synth_process(), fluidsynth pops these events from the queue and synthesize them
 *
 * during 1) we observe the events sucked from a midi file. if we spot a MIDI CC102 (or 103), we've detected a midi track loop. so here we have to make sure,
 * that fluidsynth calls us back whenever we reach the end of such a track loop during synthesization, in order to keep this midi track playing
 *
 * TODO: this class should actually be split up into handling the bare SMF part and an extra synthesizer class
 */


string FluidsynthWrapper::SmfEventToString(smf_event_t* event)
{
    string ret = "event no.   : " + to_string(event->event_number);
    ret +=     "\nin track no.: " + to_string(event->track_number);
    ret +=     "\nat tick     : " + to_string(event->time_pulses);

    return ret;
}

void FluidsynthWrapper::scheduleTrackLoop(unsigned int time, fluid_event_t* e, fluid_sequencer_t* seq, void* data)
{
    (void)seq;

    FluidsynthWrapper* pthis = static_cast<FluidsynthWrapper*>(data);
    MidiLoopInfo* loopInfo = static_cast<MidiLoopInfo*>(fluid_event_get_data(e));

    if(pthis==nullptr || loopInfo == nullptr)
    {
        return;
    }

    // seek back to where the loop started
    if(smf_seek_to_seconds(pthis->smf, loopInfo->start.Value) != 0)
    {
        CLOG(LogLevel::ERROR, "unable to seek to "<< loopInfo->start.Value << " seconds.");
        return;
    }


    fluid_event_t *fluidEvt = new_fluid_event();
    fluid_event_set_source(fluidEvt, -1);
    fluid_event_set_dest(fluidEvt, pthis->synthSeqId);

    smf_event_t* event;
    // read in every single event following the position we currently are
    while((event = smf_get_next_event(pthis->smf)) != nullptr)
    {
        // does this event belongs to the same track and plays on the same channel as where we found the corresponding loop event?
        if(event->track_number == loopInfo->trackId && (event->midi_buffer[0] & 0x0F) == loopInfo->channel)
        {
            if(IsControlChange(event) && IsLoopStop(event) && (event->midi_buffer[2] == loopInfo->loopId)) // is that our corresponding loop stop?
            {
                int ret = pthis->scheduleNextCallback(event, time-static_cast<unsigned int>(loopInfo->start.Value*1000), loopInfo);
                if(ret != FLUID_OK)
                {
                    CLOG(LogLevel::ERROR, "fluidsynth was unable to queue midi event");
                }
                break;
            }
            else if(IsControlChange(event) && IsLoopStart(event))
            {
                continue;
            }
            else
            {
                pthis->feedToFluidSeq(event, fluidEvt, loopInfo->start.Value);
            }
        }
    }

    delete_fluid_event(fluidEvt);
}

FluidsynthWrapper::FluidsynthWrapper(string filename) : StandardWrapper(filename)
{
    this->initAttr();
}

FluidsynthWrapper::FluidsynthWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen) : StandardWrapper(filename, fileOffset, fileLen)
{
    this->initAttr();
}

void FluidsynthWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::float32;
}

FluidsynthWrapper::~FluidsynthWrapper ()
{
    this->releaseBuffer();
    this->close();
}

void FluidsynthWrapper::setupSeq()
{
    this->sequencer = new_fluid_sequencer2(false /*i.e. use sample timer*/);

    // register synth as first destination
    this->synthSeqId = fluid_sequencer_register_fluidsynth(this->sequencer, this->synth);

    // register myself as second destination
    this->mySeqID = fluid_sequencer_register_client(this->sequencer, "schedule_loop_callback", &FluidsynthWrapper::scheduleTrackLoop, this);
}

void FluidsynthWrapper::setupSynth()
{
    /* Create the synthesizer */
    this->synth = new_fluid_synth(this->settings);
    if (this->synth == nullptr)
    {
        THROW_RUNTIME_ERROR("Failed to create the synth");
    }
    fluid_synth_set_reverb(this->synth, gConfig.FluidsynthRoomSize, gConfig.FluidsynthDamping, gConfig.FluidsynthWidth, gConfig.FluidsynthLevel);

    // retrieve this after the synth has been inited (just for sure)
    fluid_settings_getint(this->settings, "audio.period-size", &gConfig.FluidsynthPeriodSize);


    // find a soundfont
    Nullable<string> soundfont;
    if(!gConfig.FluidsynthForceDefaultSoundfont)
    {
        soundfont = ::findSoundfont(this->Filename);
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

    /* Load the soundfont */
    if (!fluid_is_soundfont(soundfont.Value.c_str()))
    {
        THROW_RUNTIME_ERROR("Specified soundfont seems to be invalid (weak test): \"" << soundfont.Value << "\"");
    }

    if (fluid_synth_sfload(this->synth, soundfont.Value.c_str(), true) == -1)
    {
        THROW_RUNTIME_ERROR("Specified soundfont seems to be invalid (strong test): \"" << soundfont.Value << "\"");
    }
}

void FluidsynthWrapper::setupSettings()
{
    this->settings = new_fluid_settings();
    if (this->settings == nullptr)
    {
        THROW_RUNTIME_ERROR("Failed to create the settings");
    }

    fluid_settings_setstr(this->settings, "synth.reverb.active", gConfig.FluidsynthEnableReverb ? "yes" : "no");
    fluid_settings_setstr(this->settings, "synth.chorus.active", gConfig.FluidsynthEnableChorus ? "yes" : "no");

    fluid_settings_setint(this->settings, "synth.min-note-length", 1);
    // only in mma mode, bank high and bank low controllers are handled as specified by MIDI standard
    fluid_settings_setstr(this->settings, "synth.midi-bank-select", "mma");

    {
        fluid_settings_setnum(this->settings, "synth.sample-rate", gConfig.FluidsynthSampleRate);
        double srate;
        fluid_settings_getnum(this->settings, "synth.sample-rate", &srate);
        
        if(this->Format.SampleRate != srate)
        {
            // the sample rate may have changed, if requested by user
            this->Format.SampleRate = srate;
            // so we have to build up the loop tree again
            this->buildLoopTree();
        }

        int stereoChannels = gConfig.FluidsynthMultiChannel ? 16 : 1;
        fluid_settings_setint(this->settings, "synth.audio-groups",    stereoChannels);
        fluid_settings_setint(this->settings, "synth.audio-channels",  stereoChannels);
        fluid_settings_getint(this->settings, "synth.audio-channels", &stereoChannels);
        this->Format.Channels = stereoChannels * 2;
    }

    // these maybe needed for fast renderer (even fluidsynth itself isnt sure about)
    fluid_settings_setstr(this->settings, "player.timing-source", "sample");
    fluid_settings_setint(this->settings, "synth.parallel-render", 0);
    fluid_settings_setint(this->settings, "synth.threadsafe-api", 0);
}

void FluidsynthWrapper::open ()
{
    // avoid multiple calls to open()
    if(this->settings!=nullptr)
    {
        return;
    }
    this->setupSettings();


    this->smf = smf_load(this->Filename.c_str());
    if (this->smf == NULL)
    {
        THROW_RUNTIME_ERROR("Something is wrong with that midi, loading failed");
    }

    double playtime = smf_get_length_seconds(this->smf);
    if(playtime<0.0)
    {
        THROW_RUNTIME_ERROR("How can playtime be negative?!?");
    }

    this->fileLen = static_cast<size_t>(playtime * 1000);

    this->trackLoops.resize(this->smf->number_of_tracks);
    for(unsigned int i = 0; i<this->trackLoops.size(); i++)
    {
        this->trackLoops[i].resize(16);
    }
    this->setupSynth();
    this->setupSeq();


    fluid_event_t *fluidEvt = new_fluid_event();
    fluid_event_set_source(fluidEvt, -1);
    fluid_event_set_dest(fluidEvt, this->synthSeqId);

    smf_event_t *event;
    while((event = smf_get_next_event(this->smf)) != nullptr)
    {
        if (smf_event_is_metadata(event) || smf_event_is_sysex(event))
        {
            continue;
        }

        if (!smf_event_is_valid(event))
        {
            CLOG(LogLevel::WARNING, "invalid midi event found, ignoring:" << FluidsynthWrapper::SmfEventToString(event));
            continue;
        }

        this->feedToFluidSeq(event, fluidEvt, 0.0);

    } // end of while

    smf_rewind(this->smf);

    delete_fluid_event(fluidEvt);
}

int FluidsynthWrapper::scheduleNextCallback(smf_event_t* event, unsigned int time, void* data)
{
    fluid_event_t* evt = new_fluid_event();
    fluid_event_set_source(evt, -1);
    fluid_event_set_dest(evt, this->mySeqID);
    fluid_event_timer(evt, data);

    unsigned int callbackdate = time;
    callbackdate += static_cast<unsigned int>(event->time_seconds * 1000);

    int ret = fluid_sequencer_send_at(this->sequencer, evt, callbackdate, true);
    delete_fluid_event(evt);

    return ret;
}

void FluidsynthWrapper::feedToFluidSeq(smf_event_t * event, fluid_event_t* fluidEvt, double offset)
{
    int ret; // fluidsynths return value

    uint16_t chan = event->midi_buffer[0] & 0x0F;
    switch (event->midi_buffer[0] & 0xF0)
    {
    case 0x80: // notoff
        fluid_event_noteoff(fluidEvt, chan, event->midi_buffer[1]);
        break;

    case 0x90:
        fluid_event_noteon(fluidEvt, chan, event->midi_buffer[1], event->midi_buffer[2]);
        break;

    case 0xA0:
        CLOG(LogLevel::DEBUG, "Aftertouch, channel " << chan << ", note " << static_cast<int>(event->midi_buffer[1]) << ", pressure " << static_cast<int>(event->midi_buffer[2]));
        CLOG(LogLevel::ERROR, "Fluidsynth does not support key specific pressure (aftertouch); discarding event");
        return;
        break;

    case 0xB0: // ctrl change
    {
        vector<MidiLoopInfo>& loops = this->trackLoops[event->track_number-1 /*because one based*/][chan];
        if(IsLoopStart(event))
        {
            // zero-based
            unsigned int loopId = event->midi_buffer[2];
            if(loops.size() <= loopId)
            {
                loops.resize(loopId+1);
            }

            MidiLoopInfo& info = loops[loopId];

            info.trackId = event->track_number;
            info.loopId = loopId;
            info.eventId = event->event_number;
            info.channel = chan;

            if(!info.start.hasValue) // avoid multiple sets
            {
                info.start = event->time_seconds;
            }

            // nothing more to do here
            return;
        }
        else if(IsLoopStop(event))
        {
            // zero-based
            unsigned int loopId = event->midi_buffer[2];
            if(loops.size() <= loopId)
            {
                CLOG(LogLevel::ERROR, "Received loop end, but there was no corresponding loop start");

                // ...well, cant do anything here
                return;
            }
            else
            {
                MidiLoopInfo& info = loops[loopId];

                if(!info.stop.hasValue)
                {
                    info.stop = event->time_seconds;
                }

                ret = this->scheduleNextCallback(event, fluid_sequencer_get_tick(this->sequencer), &info);
                goto CHECK_RETURN_VALUE;
            }
        }
        else // just a usual control change
        {
            fluid_event_control_change(fluidEvt, chan, event->midi_buffer[1], event->midi_buffer[2]);
        }

        CLOG(LogLevel::DEBUG, "Controller, channel " << chan << ", controller " << static_cast<int>(event->midi_buffer[1]) << ", value " << static_cast<int>(event->midi_buffer[2]));
    }
    break;

    case 0xC0:
        fluid_event_program_change(fluidEvt, chan, event->midi_buffer[1]);
        CLOG(LogLevel::DEBUG, "ProgChange, channel " << chan << ", program " << static_cast<int>(event->midi_buffer[1]));
        break;

    case 0xD0:
        fluid_event_channel_pressure(fluidEvt, chan, event->midi_buffer[1]);
        CLOG(LogLevel::DEBUG, "Channel Pressure, channel " << chan << ", pressure " << static_cast<int>(event->midi_buffer[1]));
        break;

    case 0xE0:
    {
        int16_t pitch = event->midi_buffer[2];
        pitch <<= 7;
        pitch |= event->midi_buffer[1];

        fluid_event_pitch_bend(fluidEvt, chan, pitch);
        CLOG(LogLevel::DEBUG, "Pitch Wheel, channel " << chan << ", value " << pitch);
    }
    break;

    default:
        return;
    }

    ret = fluid_sequencer_send_at(this->sequencer, fluidEvt, static_cast<unsigned int>((event->time_seconds-offset)*1000) + fluid_sequencer_get_tick(this->sequencer), true);

CHECK_RETURN_VALUE:
    if(ret != FLUID_OK)
    {
        CLOG(LogLevel::ERROR, "fluidsynth was unable to queue midi event");
    }
}

void FluidsynthWrapper::close() noexcept
{
    if(this->smf != nullptr)
    {
        smf_delete(this->smf);
        this->smf = nullptr;
    }

    if(this->sequencer != nullptr)
    {
        delete_fluid_sequencer(this->sequencer);
        this->sequencer = nullptr;
    }
    
    if(this->synth != nullptr)
    {
        delete_fluid_synth(this->synth);
        this->synth = nullptr;
    }
    
    if(this->settings != nullptr)
    {
        delete_fluid_settings(this->settings);
        this->settings = nullptr;
    }
}

void FluidsynthWrapper::fillBuffer()
{
    StandardWrapper::fillBuffer(this);
}

frame_t FluidsynthWrapper::getFrames () const
{
    return msToFrames(this->fileLen.Value, this->Format.SampleRate);
}


vector<loop_t> FluidsynthWrapper::getLoopArray () const noexcept
{
    // so, here we are, having a bunch of individually looped midi tracks (maybe)
    // somehow trying to put those loops together so that we find a valid master loop within the generated PCM

    const MidiLoopInfo* max = nullptr;


    for(unsigned int t=0; t<this->trackLoops.size(); t++)
    {
        for(unsigned int c=0; c<this->trackLoops[t].size(); c++)
        {
            for(unsigned int l=0; l<this->trackLoops[t][c].size(); l++)
            {
                const MidiLoopInfo& info = this->trackLoops[t][c][l];

                if(info.start.hasValue && info.stop.hasValue)
                {
                    double duration = info.stop.Value - info.start.Value;
                    if(max == nullptr)
                    {
                        max = &info;
                    }
                    else
                    {
                        double durationMax = max->stop.Value - max->start.Value;
                        if(durationMax < duration)
                        {
                            max=&info;
                        }
                    }
                }
            }
        }
    }

    vector<loop_t> loopArr;

    if(max != nullptr)
    {
        loop_t l;
        l.start = ::msToFrames(static_cast<size_t>(max->start.Value*1000), this->Format.SampleRate);
        l.stop = ::msToFrames(static_cast<size_t>(max->stop.Value*1000), this->Format.SampleRate);
        l.count = max->count;

        loopArr.push_back(l);
    }

    return loopArr;
}

// HACK there seems to be some strange bug in fluidsynth:
// whenever we ask the synth to render something else than exactly 64 frames, we get strangely timed audio
// thus obtain the period size (?always? == 64) within this->open() and use this value instead of our gConfig.FramesToRender
// by replacing gConfig.FramesToRender with gConfig.FluidsynthPeriodSize
#define FramesToRender FluidsynthPeriodSize
void FluidsynthWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    float** temp_buf = new float*[this->Format.Channels];

    for(unsigned int i = 0; i< this->Format.Channels; i++)
    {
        temp_buf[i] = new float[gConfig.FramesToRender];
    }

    STANDARDWRAPPER_RENDER(float,

                           fluid_synth_process(this->synth, framesToDoNow, 0, nullptr, this->Format.Channels, temp_buf);
                           for(int frame=0; frame<framesToDoNow; frame++)
                           for(unsigned int c=0; c<this->Format.Channels; c++)
                           {
                                pcm[frame * this->Format.Channels + c] = temp_buf[c][frame];
                           };
                          )

    for(unsigned int i=0; i < this->Format.Channels; i++)
    {
        delete[] temp_buf[i];
    }

    delete[] temp_buf;
}
#undef FramesToRender
