#include "MidiWrapper.h"
#include "FluidsynthWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Config.h"


#include <chrono>
#include <cmath>
#include <cstring>
#include <thread> // std::this_thread::sleep_for
#include <utility>
#include <fstream>
#include <bitset>

#define IsControlChange(e) ((e->midi_buffer[0] & 0xF0) == 0xB0)
#define IsLoopStart(e) (IsControlChange(e) && (e->midi_buffer[1] == gConfig.MidiControllerLoopStart))
#define IsLoopStop(e) (IsControlChange(e) && (e->midi_buffer[1] == gConfig.MidiControllerLoopStop))
#define IsLoopCount(e) (IsControlChange(e) && (e->midi_buffer[1] == gConfig.MidiControllerLoopCount))


/** class MidiWrapper
 *
 * Things we are doing here:
 *
 *  1. use libsmf to retrieve midi events from a midi file
 *  2. feed these event to some synthesizer (i.e. fluidsynth's sequencer)
 *  3. the synth manages an internal queue. on every call to this->render(), the synth pops these events from the queue and synthesize them
 *
 * during 1) we observe the events sucked from a midi file. if we spot a MIDI CC102 (or 103), we've detected a midi track loop. so here we have to make sure,
 * that we get a callback whenever we reach the end of such a track loop during synthesization, in order to keep this midi track playing
 *
 */


string MidiWrapper::SmfEventToString(smf_event_t *event)
{
    string ret = "event no.   : " + std::to_string(event->event_number);
    ret += "\nin track no.: " + std::to_string(event->track_number);
    ret += "\nat tick     : " + std::to_string(event->time_pulses);

    return ret;
}

MidiWrapper::MidiWrapper(string filename)
: StandardWrapper(std::move(filename))
{
    this->initAttr();
}

MidiWrapper::MidiWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen)
: StandardWrapper(std::move(filename), fileOffset, fileLen)
{
    this->initAttr();
}

void MidiWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::float32;
    this->lastOverridingLoopCount = gConfig.overridingGlobalLoopCount;
    this->lastUseLoopInfo = gConfig.useLoopInfo;
}

// calling libsmf, parsing through all events is expensive
// the idea is to outsource those long-running parts and only do them when really necessary, i.e. when creating this object and whenever the user changes gConfig.overridingGlobalLoopCount
void MidiWrapper::initialize()
{
    std::ifstream file(this->Filename, std::ios::binary | std::ios::ate);

    if(file)
    {
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size))
        {
            THROW_RUNTIME_ERROR("Unable to read midi " << this->Filename);
        }

        if (this->smf != nullptr)
        {
            smf_delete(this->smf);
        }
        this->smf = smf_load_from_memory(buffer.data(), size);
        if (this->smf == nullptr)
        {
            THROW_RUNTIME_ERROR("Something is wrong with that midi, loading failed");
        }

        double playtime = smf_get_length_seconds(this->smf);
        if (playtime <= 0.0)
        {
            THROW_RUNTIME_ERROR("How can playtime be negative?!?");
        }
        this->fileLen = static_cast<size_t>(playtime * 1000);

        this->trackLoops.clear();
        this->trackLoops.resize(this->smf->number_of_tracks);
    }
    else
    {
        THROW_RUNTIME_ERROR("Unable to open midi " << this->Filename);
    }
}

MidiWrapper::~MidiWrapper()
{
    this->releaseBuffer();
    this->close();
}

void MidiWrapper::open()
{
    this->synth = new FluidsynthWrapper();
    this->synth->Init(::findSoundfont(this->Filename));

    this->Format.SampleRate = this->synth->GetSampleRate();

    if(gConfig.overridingGlobalLoopCount != this->lastOverridingLoopCount
        || gConfig.useLoopInfo != this->lastUseLoopInfo)
    {
        this->initAttr();
    }

    this->initialize();
    this->synth->SetDefaultSeqTempoScale(this->smf->ppqn);
    this->parseEvents();

    auto finishAtTick = smf_get_length_pulses(this->smf);
    const auto* max = this->getLongestMidiTrackLoop();
    if(this->lastUseLoopInfo && max != nullptr)
    {
        this->fileLen.Value += (max->stop.Value - max->start.Value)*1000 * std::max(0, this->lastOverridingLoopCount-1);
        finishAtTick += (max->stop_tick.Value - max->start_tick.Value)   * std::max(0, this->lastOverridingLoopCount-1);
    }

    // finally send note offs on all channels
    // we have to do this, because some wind might be blowing (DK64)
    this->synth->FinishSong(finishAtTick);
    this->synth->ConfigureChannels(&this->Format);


    this->buildLoopTree();
}

void MidiWrapper::parseEvents()
{
    // N64 SDK section 20.5.1: "A loop count of zero, the default, will loop forever."
    int lastestLoopCount = 0;

    smf_rewind(this->smf);
    smf_event_t *event;
    while ((event = smf_get_next_event(this->smf)) != nullptr)
    {
        if (!smf_event_is_valid(event))
        {
            CLOG(LogLevel_t::Warning, "invalid midi event found, ignoring:" << MidiWrapper::SmfEventToString(event));
            continue;
        }

        if (this->lastUseLoopInfo)
        {
            if (IsLoopCount(event))
            {
                lastestLoopCount = event->midi_buffer[2];
                continue;
            }

            vector<MidiLoopInfo> &loops = this->trackLoops[event->track_number - 1 /*because one based*/];
            if (IsLoopStart(event))
            {
                // zero-based
                unsigned int loopId = event->midi_buffer[2];
                if (loops.size() <= loopId)
                {
                    loops.resize(loopId + 1);
                }

                MidiLoopInfo &info = loops[loopId];

                info.trackId = event->track_number;
                info.eventId = event->event_number;
                info.loopId = loopId;

                if (!info.start.hasValue) // avoid multiple sets
                {
                    info.start = event->time_seconds;
                    info.start_tick = event->time_pulses;
                }

                continue;
            }

            if (IsLoopStop(event))
            {
                // zero-based
                unsigned int loopId = event->midi_buffer[2];
                if (loops.size() <= loopId)
                {
                    CLOG(LogLevel_t::Warning, "Received loop end, but there was no corresponding loop start");

                    // ...well, cant do anything here
                    continue;
                }
                else
                {
                    MidiLoopInfo &info = loops[loopId];

                    if (!info.stop.hasValue)
                    {
                        info.stop = event->time_seconds;
                        info.stop_tick = event->time_pulses;
                        info.count = lastestLoopCount;

                        // collect all events that are part of this loop, as well as note off events that are already outside
                        // of the loop but belong to note on events that have been triggered within the loop.
                        //
                        // This workaround is required because for some ambiance tunes in DK64 the noteoff events happen 
                        // after the loopend. As a consequence, notes that have been turned on during the loop will never be stopped.
                        {
                            std::bitset<128> noteIsPlaying;
                            int numberOfEvents = event->track->number_of_events;
                            info.eventsInLoop.reserve(numberOfEvents);

                            // collect all events that are part of the MIDI track loop
                            for(int i=0; i < numberOfEvents; i++)
                            {
                                smf_event_t *evt_of_loop = smf_track_get_event_by_number(event->track, i+1);

                                // does this event belong to the same track as where we found the corresponding loop event?
                                if (evt_of_loop != nullptr &&
                                    evt_of_loop->track_number == info.trackId && //redundant?
                                    info.start_tick.Value <= evt_of_loop->time_pulses &&
                                    !((IsLoopStart(evt_of_loop)) || (IsLoopCount(evt_of_loop)) || (IsLoopStop(evt_of_loop))))
                                {
                                    if (!smf_event_is_metadata(event))
                                    {
                                        // events shall not be looped beyond the end of the song
                                        unsigned char key = evt_of_loop->midi_buffer[1];

                                        if (evt_of_loop->time_pulses >= info.stop_tick.Value)
                                        {
                                            if(noteIsPlaying.none())
                                            {
                                                // all events of this loop handled, stop here
                                                break;
                                            }
                                            else if(!((evt_of_loop->midi_buffer[0] & 0xF0) == 0x80
                                                ||  ((evt_of_loop->midi_buffer[0] & 0xF0) == 0x90
                                                    && evt_of_loop->midi_buffer[2] == 0)))
                                            {
                                                // smth. else than noteoff
                                                continue;
                                            }
                                            else if(!noteIsPlaying.test(key))
                                            {
                                                // not the noteoff we are looking for
                                                continue;
                                            }
                                        }

                                        // keep track of all notes that have been turned on by this current MIDI track loop and continue searching for a noteoff event beyound the loopend
                                        switch (evt_of_loop->midi_buffer[0] & 0xF0)
                                        {
                                            NOTEOFF:
                                            case 0x80: // noteoff
                                                noteIsPlaying.reset(key);
                                                break;

                                            case 0x90:
                                                if(evt_of_loop->midi_buffer[2] == 0)
                                                {
                                                    goto NOTEOFF;
                                                }
                                                noteIsPlaying.set(key);
                                                [[fallthrough]];
                                            default:
                                                break;
                                        }
                                    }

                                    // this event is confirmed to be part of the loop. save it.
                                    info.eventsInLoop.push_back(evt_of_loop);
                                }
                            }
                        }
                        this->synth->ScheduleLoop(&info);
                    }
                }
            }
        }

        this->synth->AddEvent(event);

    } // end of while

    smf_rewind(this->smf);
}


void MidiWrapper::close() noexcept
{
    if (this->synth != nullptr)
    {
        delete this->synth;
        this->synth = nullptr;
    }

    if (this->smf != nullptr)
    {
        smf_delete(this->smf);
        this->smf = nullptr;
    }
}

frame_t MidiWrapper::getFrames() const
{
    size_t len = this->fileLen.Value;
    if((this->synth != nullptr && this->synth->GetReverbActive()) || (this->synth == nullptr && gConfig.FluidsynthEnableReverb))
    {
        len += 1500u;
    }

    return (this->Format.Voices == 0) ? 0 : msToFrames(len, this->Format.SampleRate);
}

const MidiLoopInfo* MidiWrapper::getLongestMidiTrackLoop() const
{
    const MidiLoopInfo *max = nullptr;
    for (unsigned int t = 0; t < this->trackLoops.size(); t++)
    {
        for (unsigned int l = 0; l < this->trackLoops[t].size(); l++)
        {
            const MidiLoopInfo &info = this->trackLoops[t][l];

            // only infinite midi track loops are considered as master loop for the entire song
            if (info.start_tick.hasValue && info.stop_tick.hasValue && info.count == 0)
            {
                int duration = info.stop_tick.Value - info.start_tick.Value;
                if (max == nullptr)
                {
                    max = &info;
                }
                else
                {
                    int durationMax = max->stop_tick.Value - max->start_tick.Value;
                    if (durationMax < duration)
                    {
                        max = &info;
                    }
                }
            }
        }
    }
    return max;
}

vector<loop_t> MidiWrapper::getLoopArray() const noexcept
{
    vector<loop_t> loopArr;

    // if the loop should be unrolled and loop info is respected, do not attemtp to search for a potential master loop and simply unroll all midi track loops
    //
    // if the loop info is used but the song is not rendered in a single buffer AND the user has requested infinite playback, also unroll everything "forever"
    if(gConfig.useLoopInfo &&
        (this->lastOverridingLoopCount >= 1
        ||
        (this->lastOverridingLoopCount <= 0
        && !gConfig.RenderWholeSong)))
    {
        return loopArr;
    }

    // so, here we are, having a bunch of individually looped midi tracks (maybe)
    // somehow trying to put those loops together so that we find a valid master loop within the generated PCM

    const MidiLoopInfo *max = this->getLongestMidiTrackLoop();
    if (max != nullptr)
    {
        loop_t l;
        l.start = ::msToFrames(static_cast<size_t>(max->start.Value * 1000), this->Format.SampleRate);
        l.stop = ::msToFrames(static_cast<size_t>(max->stop.Value * 1000), this->Format.SampleRate);
        l.count = max->count;

        loopArr.push_back(l);
    }

    return loopArr;
}

void MidiWrapper::render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender)
{
    STANDARDWRAPPER_RENDER(float, this->synth->Render(pcm, framesToDoNow))
}
