#include "MidiWrapper.h"
#include "FluidsynthWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#define IsControlChange(e) ((e->midi_buffer[0] & 0xF0) == 0xB0)
#define IsLoopStart(e) IsControlChange(e) && (e->midi_buffer[1] == gConfig.MidiControllerLoopStart)
#define IsLoopStop(e)  IsControlChange(e) && (e->midi_buffer[1] == gConfig.MidiControllerLoopStop)
#define IsLoopCount(e) IsControlChange(e) && (e->midi_buffer[1] == gConfig.MidiControllerLoopCount)


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


string MidiWrapper::SmfEventToString(smf_event_t* event)
{
    string ret = "event no.   : " + to_string(event->event_number);
    ret +=     "\nin track no.: " + to_string(event->track_number);
    ret +=     "\nat tick     : " + to_string(event->time_pulses);

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
                    if(event->midi_buffer[2] == loopInfo->loopId)
                    {
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

MidiWrapper::MidiWrapper(string filename) : StandardWrapper(filename)
{
    this->initAttr();
}

MidiWrapper::MidiWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen) : StandardWrapper(filename, fileOffset, fileLen)
{
    this->initAttr();
}

void MidiWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::float32;
}

MidiWrapper::~MidiWrapper ()
{
    this->releaseBuffer();
    this->close();
    
    delete this->synth;
    this->synth = nullptr;
}


void MidiWrapper::open ()
{
    if(this->smf != nullptr)
    {
        return;
    }
    
    this->smf = smf_load(this->Filename.c_str());
    if (this->smf == NULL)
    {
        THROW_RUNTIME_ERROR("Something is wrong with that midi, loading failed");
    }

//     if(!this->fileLen.hasValue)
    {
        double playtime = smf_get_length_seconds(this->smf);
        if(playtime<0.0)
        {
            THROW_RUNTIME_ERROR("How can playtime be negative?!?");
        }
        this->fileLen = static_cast<size_t>(playtime * 1000);
    }
    
    this->trackLoops.clear();
    this->trackLoops.resize(this->smf->number_of_tracks);
    for(unsigned int i = 0; i<this->trackLoops.size(); i++)
    {
        this->trackLoops[i].clear();
        this->trackLoops[i].resize(NMidiChannels);
    }
    
    
    this->synth = new FluidsynthWrapper();
    this->synth->ShallowInit();
    
    this->synth->ConfigureChannels(&this->Format);
    
    unsigned int srate = this->synth->GetSampleRate();
    if(this->Format.SampleRate != srate)
    {
        // the sample rate may have changed, if requested by user
        this->Format.SampleRate = srate;
        // so we have to build up the loop tree again
        this->buildLoopTree();
    }
}

void MidiWrapper::parseEvents()
{
    int lastestLoopCount[NMidiChannels] = {};
    smf_event_t *event;
    while((event = smf_get_next_event(this->smf)) != nullptr)
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
        
        uint16_t chan = event->midi_buffer[0] & 0x0F;
        if(IsLoopCount(event))
        {
            lastestLoopCount[chan] = event->midi_buffer[2];
            continue;
        }
        
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
            info.eventId = event->event_number;
            info.channel = chan;
            info.loopId = loopId;
            info.count = lastestLoopCount[chan];

            if(!info.start.hasValue) // avoid multiple sets
            {
                info.start = event->time_seconds;
            }
            
            continue;
        }
        
        if(IsLoopStop(event))
        {
            // zero-based
            unsigned int loopId = event->midi_buffer[2];
            if(loops.size() <= loopId)
            {
                CLOG(LogLevel_t::Error, "Received loop end, but there was no corresponding loop start");

                // ...well, cant do anything here
                continue;
            }
            else
            {
                MidiLoopInfo& info = loops[loopId];

                if(!info.stop.hasValue)
                {
                    info.stop = event->time_seconds;
                }

                this->synth->ScheduleLoop(&info);
            }
        }

        this->synth->AddEvent(event);

    } // end of while

    smf_rewind(this->smf);
    
    // finally send note offs on all channels
    // we have to do this, because some wind might be blowing (DK64)
    const int& endOfSong = this->fileLen.Value;
    this->synth->FinishSong(endOfSong);
}


void MidiWrapper::close() noexcept
{
    if(this->synth != nullptr)
    {
        this->synth->Unload();
//         delete this->synth;
//         this->synth = nullptr;
    }
    
    if(this->smf != nullptr)
    {
        smf_delete(this->smf);
        this->smf = nullptr;
    }
}

void MidiWrapper::fillBuffer()
{
    if(this->data==nullptr)
    {
        this->synth->DeepInit(*this);
        // if this is the first call, add the midi events to the sequencer
        this->parseEvents();
        this->buildLoopTree();
    }
    
    StandardWrapper::fillBuffer(this);
}

frame_t MidiWrapper::getFrames () const
{
    size_t len = this->fileLen.Value;
    len += 3000;
//     if(gConfig.FluidsynthEnableReverb)
//     {
//         len += (gConfig.FluidsynthRoomSize*10.0 * gConfig.FluidsynthLevel * 1000) / 2;
//     }
    
    return msToFrames(len, this->Format.SampleRate);
}


vector<loop_t> MidiWrapper::getLoopArray () const noexcept
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

void MidiWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    STANDARDWRAPPER_RENDER(float, this->synth->Render(pcm, framesToDoNow))
    
    this->doAudioNormalization(static_cast<float*>(bufferToFill), framesToRender);
}
