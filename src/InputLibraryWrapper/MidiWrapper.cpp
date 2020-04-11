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
    string ret = "event no.   : " + to_string(event->event_number);
    ret += "\nin track no.: " + to_string(event->track_number);
    ret += "\nat tick     : " + to_string(event->time_pulses);

    return ret;
}

/**
 * @param time value returned by fluid_sequencer_get_tick(), i.e. usually time in milliseconds
 */
void MidiWrapper::FluidSeqCallback(unsigned int time, fluid_event_t* e, fluid_sequencer_t* seq, void* data)
{
    (void)seq;

    MidiWrapper* pthis = static_cast<MidiWrapper*>(data);
    MidiLoopInfo* loopInfo = static_cast<MidiLoopInfo*>(fluid_event_get_data(e));

    if(pthis==nullptr || loopInfo == nullptr)
    {
        return;
    }

    // seek back to where the loop started
    if(smf_seek_to_seconds(pthis->smf, loopInfo->start.Value) != 0)
    {
        CLOG(LogLevel_t::Error, "unable to seek to "<< loopInfo->start.Value << " seconds.");
        return;
    }

    // the sequencer's tick is not necessarily resetted when playing the next midi file, however, we need his tick to perform that check down there
    // thus, set it relative to the beginning of the current midi
    time -= pthis->synth->GetInitTick();
    
    smf_event_t* event;
    // read in every single event following the position we currently are
    while((event = smf_get_next_event(pthis->smf)) != nullptr)
    {
        // does this event belong to the same track and plays on the same channel as where we found the corresponding loop event?
        if(event->track_number == loopInfo->trackId && (event->midi_buffer[0] & 0x0F) == loopInfo->channel)
        {
            // events shall not be looped beyond the end of the song
            if(time + event->time_seconds*1000 < pthis->fileLen.Value)
            {
                if(IsLoopStop(event))
                {
                    // is that our corresponding loop stop?
                    if(event->midi_buffer[2] == loopInfo->loopId &&
                        // is there still loop count left? 2 because one loop was already scheduled by parseEvents() and 0 is excluded
                       (loopInfo->count == 0 || loopInfo->count > 2)
                    )
                    {
                        loopInfo->count--;
                        pthis->synth->ScheduleLoop(loopInfo);
                        break;
                    }
                    else
                    {
                        // dont pass loop events to the synth
                        continue;
                    }
                }
                else if(IsLoopStart(event))
                {
                    continue;
                }
                else
                {
                    pthis->synth->AddEvent(event);
                }
            }
        }
    }
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
    this->initialize();
}

// calling libsmf, parsing through all events is expensive
// the idea is to outsource those long-running parts and only do them when really necessary, i.e. when creating this object and whenever the user changes gConfig.overridingGlobalLoopCount
void MidiWrapper::initialize()
{
    std::ifstream file(this->Filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
    {
        THROW_RUNTIME_ERROR("Unable to load midi " << this->Filename);
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
    for (unsigned int i = 0; i < this->trackLoops.size(); i++)
    {
        this->trackLoops[i].clear();
        this->trackLoops[i].resize(NMidiChannels);
    }

    this->parseEvents();

    const auto* max = this->getLongestMidiTrackLoop();
    if(gConfig.useLoopInfo && max != nullptr)
    {
        this->fileLen.Value += (max->stop.Value - max->start.Value)*1000 * std::max(0, this->lastOverridingLoopCount-1);
    }
}

MidiWrapper::~MidiWrapper()
{
    this->releaseBuffer();
    this->close();

    if (this->smf != nullptr)
    {
        smf_delete(this->smf);
        this->smf = nullptr;
    }
}

void MidiWrapper::open()
{
    if(gConfig.overridingGlobalLoopCount != this->lastOverridingLoopCount
        || gConfig.useLoopInfo != this->lastUseLoopInfo)
    {
        this->initAttr();
    }

    this->synth = new FluidsynthWrapper(::findSoundfont(this->Filename), *this);

    smf_rewind(this->smf);
    smf_event_t *event;
    while ((event = smf_get_next_event(this->smf)) != nullptr)
    {
        if (!smf_event_is_valid(event)
            || smf_event_is_metadata(event)
            || smf_event_is_sysex(event)
            || IsLoopCount(event)
            || IsLoopStart(event)
            || IsLoopStop(event)
            || static_cast<unsigned int>((event->time_seconds * 1000) >= this->fileLen.Value))
        {
            continue;
        }

        this->synth->AddEvent(event);
    } // end of while

    // finally send note offs on all channels
    // we have to do this, because some wind might be blowing (DK64)
    this->synth->FinishSong(this->fileLen.Value);

    this->synth->ConfigureChannels(&this->Format);

    this->Format.SampleRate = this->synth->GetSampleRate();
    this->buildLoopTree();
}

void MidiWrapper::parseEvents()
{
    int lastestLoopCount[NMidiChannels] = {};
    const double endOfSong = this->fileLen.Value * std::max(1, this->lastOverridingLoopCount) / 1000;

    smf_event_t *event;
    while ((event = smf_get_next_event(this->smf)) != nullptr)
    {
        if (smf_event_is_metadata(event) || smf_event_is_sysex(event))
        {
            continue;
        }

        if (!smf_event_is_valid(event))
        {
            CLOG(LogLevel_t::Warning, "invalid midi event found, ignoring:" << MidiWrapper::SmfEventToString(event));
            continue;
        }

        if (!this->lastUseLoopInfo)
        {
            continue;
        }

        uint16_t chan = event->midi_buffer[0] & 0x0F;
        if (IsLoopCount(event))
        {
            lastestLoopCount[chan] = event->midi_buffer[2];
            continue;
        }

        vector<MidiLoopInfo> &loops = this->trackLoops[event->track_number - 1 /*because one based*/][chan];
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
            info.channel = chan;
            info.loopId = loopId;

            if (!info.start.hasValue) // avoid multiple sets
            {
                info.start = event->time_seconds;
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
                    info.count = lastestLoopCount[chan];
                }

                this->synth->ScheduleLoop(&info);
            }
        }

        this->synth->AddEvent(event);

    } // end of while

    smf_rewind(this->smf);
    
    // finally send note offs on all channels
    // we have to do this, because some wind might be blowing (DK64)
    this->synth->FinishSong(endOfSong);
}


void MidiWrapper::close() noexcept
{
    if (this->synth != nullptr)
    {
        delete this->synth;
        this->synth = nullptr;
    }
}

frame_t MidiWrapper::getFrames() const
{
    size_t len = this->fileLen.Value;
    len += 3000;
    //     if(gConfig.FluidsynthEnableReverb)
    //     {
    //         len += (gConfig.FluidsynthRoomSize*10.0 * gConfig.FluidsynthLevel * 1000) / 2;
    //     }

    return (this->Format.Voices == 0) ? 0 : msToFrames(len, this->Format.SampleRate);
}

const MidiLoopInfo* MidiWrapper::getLongestMidiTrackLoop() const
{
    const MidiLoopInfo *max = nullptr;
    for (unsigned int t = 0; t < this->trackLoops.size(); t++)
    {
        for (unsigned int c = 0; c < this->trackLoops[t].size(); c++)
        {
            for (unsigned int l = 0; l < this->trackLoops[t][c].size(); l++)
            {
                const MidiLoopInfo &info = this->trackLoops[t][c][l];

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

    this->doAudioNormalization(static_cast<float *>(bufferToFill), framesToRender);
}
