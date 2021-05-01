#include "N64CSeqWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Config.h"


#include <bitset>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <thread> // std::this_thread::sleep_for
#include <utility>

#define _BSD_SOURCE
#include <endian.h>

// Why implementing this? Isn't Subdrags' N64MidiTool enough?
//
// - MIDI uses different volume envelopes than N64
// - Grant Kirkhope's DK64 tunes are technically broken: He uses a MIDI track loop to repeatedly trigger insects in JungleJapes. However, the notes have a duration that is longer than the duration of the track loop. In N64 CSeq this is not audible, because MIDI Notes have a duration.
// - JFG makes use of overlapping notes to increase the volume of an instrument. According to the MIDI spec, overlapping notes cancel out each other.
// - BT uses up to 32 channels, but MIDI only provides 16

// N64 CSeq uses duration for notes. But sadly, even this is broken. Sparse0x28 of DK64:
// Tick 5: NoteOn Event chan: 1 key: 45 vel: 127 dur: 187 <--
// Tick 96: NoteOn Event chan: 1 key: 45 vel: 127 dur: 5  <--
// Tick 192: NoteOn Event chan: 1 key: 44 vel: 127 dur: 96
//
// but it should be something like:
// Tick 5: NoteOn Event chan: 1 key: 45 vel: 127 dur: 96
// Tick 96: NoteOn Event chan: 1 key: 45 vel: 127 dur: 96
// Tick 192: NoteOn Event chan: 1 key: 44 vel: 127 dur: 96

N64CSeqWrapper::N64CSeqWrapper(string filename)
: StandardWrapper(std::move(filename))
{
    this->initAttr();
}

N64CSeqWrapper::N64CSeqWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen)
: StandardWrapper(std::move(filename), fileOffset, fileLen)
{
    this->initAttr();
}

void N64CSeqWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::float32;
    this->lastOverridingLoopCount = gConfig.overridingGlobalLoopCount;
    this->lastUseLoopInfo = gConfig.useLoopInfo;
}

N64CSeqWrapper::~N64CSeqWrapper()
{
    this->releaseBuffer();
    this->close();
}

static uint8_t getTrackByte(CSeqState *seq, uint32_t track)
{
    uint8_t theByte;

    if (seq->curBckLen[track])
    {
        theByte = *seq->curBckp[track]++;
        seq->curBckLen[track]--;
    }
    else /* need to handle backup mode */
    {
        theByte = *seq->cur[track]++;
        if (theByte == CMIDI_BLOCK_CODE)
        {
            uint8_t loBackUp, hiBackUp, theLen, nextByte;
            uint32_t backup;

            nextByte = *seq->cur[track]++;
            if (nextByte != CMIDI_BLOCK_CODE)
            {
                /* if here, then got a backup section. get the amount of
                   backup, and the len of the section. Subtract the amount of
                   backup from the cur ptr, and subtract four more, since
                   cur has been advanced by four while reading the codes. */
                hiBackUp = nextByte;
                loBackUp = *seq->cur[track]++;
                theLen = *seq->cur[track]++;
                backup = hiBackUp;
                backup <<= 8;
                backup += loBackUp;
                seq->curBckp[track] = seq->cur[track] - (backup + 4);
                seq->curBckLen[track] = theLen;

                /* now get the byte */
                theByte = *seq->curBckp[track]++;
                seq->curBckLen[track]--;
            }
        }
    }

    return theByte;
}

static uint32_t readVarLen(CSeqState *seq, uint32_t track)
{
    uint32_t value;
    uint32_t c;

    value = getTrackByte(seq, track);
    if (value & 0x00000080)
    {
        value &= 0x7f;
        do
        {
            c = getTrackByte(seq, track);
            value = (value << 7) + (c & 0x7f);
        } while (c & 0x80);
    }
    return (value);
}

/*
  Note: If there are no valid tracks (ie. all tracks have
  reached the end of their data stream), then return false
  to indicate that there is no next event.
*/
static bool cSeqNextDelta(CSeqState *seq)
{
    uint32_t i, firstTime = 0xFFFFFFFF;
    uint32_t lastTicks = seq->lastDeltaTicks;

    if (!seq->validTracks)
    {
        return false;
    }

    for (i = 0; i < NMidiChannels; i++)
    {
        if ((seq->validTracks >> i) & 1)
        {
            if (seq->deltaFlag)
            {
                seq->evtDeltaTicks[i] -= lastTicks;
            }

            if (seq->evtDeltaTicks[i] < firstTime)
            {
                firstTime = seq->evtDeltaTicks[i];
            }
        }
    }
    seq->deltaFlag = false;

    return true;
}

/*
 Calculates the delta time in ticks until the next sequence
 event. Loops are handled automatically by the compact sequence.
*/
static bool cSeqNextSeqEvent(CSeqState *seq)
{
    if (seq == nullptr)
    {
        return false;
    }

    /* Get the next event time in ticks. */
    /* If false is returned, then there is no next delta (ie. end of sequence reached). */
    return cSeqNextDelta(seq);
}

/* only call alCSeqGetTrackEvent with a valid track !! */
uint32_t N64CSeqWrapper::cSeqGetTrackEvent(CSeqState *seq, uint32_t track, CSeqEvent *event, bool ignoreLoops)
{
    uint8_t status, loopCt, curLpCt, *tmpPtr;

    status = getTrackByte(seq, track); /* read the status byte */

    if (status == MIDI_Meta) /* running status not allowed on meta events!! */
    {
        uint8_t type = getTrackByte(seq, track);

        if (type == MIDI_META_TEMPO)
        {
            event->type = Evt::TEMPO;
            event->msg.tempo.status = status;
            event->msg.tempo.type = type;
            event->msg.tempo.byte1 = getTrackByte(seq, track);
            event->msg.tempo.byte2 = getTrackByte(seq, track);
            event->msg.tempo.byte3 = getTrackByte(seq, track);
            seq->lastStatus[track] = 0; /* lastStatus not supported after meta */
        }
        else if (type == MIDI_META_EOT)
        {
            uint32_t flagMask = 0x01 << track;
            seq->validTracks &= ~flagMask;

            if (seq->validTracks)
            { /* there is music left don't end */
                event->type = Evt::TRACK_END;
            }
            else
            { /* no more music send SEQ_END msg */
                event->type = Evt::SEQ_END;
            }
        }
        else if (type == CMIDI_LOOPSTART_CODE)
        {
            int x = getTrackByte(seq, track); /* get next two bytes, ignore them */
            int y = getTrackByte(seq, track);

            CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": Loop Start Event track: " << track << " (" << x << " | " << y << ")");

            seq->lastStatus[track] = 0;
            seq->lastLoopStartId[track].push(x);
            event->msg.loop.id = x;
            event->msg.loop.track = track;
            event->type = Evt::LOOPSTART;
        }
        else if (type == CMIDI_LOOPEND_CODE)
        {
            tmpPtr = seq->cur[track];
            loopCt = *tmpPtr++;
            curLpCt = *tmpPtr;
            CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": Loop Stop Event track: " << track << " count: " << (int)loopCt);
            if (ignoreLoops || curLpCt == 0) /* done looping */
            {
                *tmpPtr = loopCt; /* reset current loop count */
                seq->cur[track] = tmpPtr + 5; /* move pointer to end of event */
            }
            else
            {
                if (curLpCt != 0xFF)
                { /* not a loop forever */
                    *tmpPtr = curLpCt - 1; /* decrement current loop count */
                }
                tmpPtr++; /* get offset from end of event */
                uint32_t offset = (*tmpPtr++) << 24;
                offset += (*tmpPtr++) << 16;
                offset += (*tmpPtr++) << 8;
                offset += *tmpPtr++;
                auto jumpTo = tmpPtr - offset; /* move pointer to end of event */

                if (jumpTo < this->fileBuf.data() ||
                    jumpTo >= this->fileBuf.data() + this->fileBuf.size())
                {
                    CLOG(LogLevel_t::Warning, "LoopEnd event specifies jump beyound file, ignoring.");
                    seq->cur[track] = tmpPtr;
                }
                else
                {
                    seq->cur[track] = jumpTo;
                }
            }
            seq->lastStatus[track] = 0;
            if (seq->lastLoopStartId[track].empty())
            {
                CLOG(LogLevel_t::Warning, "Received LoopEnd, but there was no LoopStart");
                event->msg.loop.id = 0;
            }
            else
            {
                event->msg.loop.id = seq->lastLoopStartId[track].top();
                seq->lastLoopStartId[track].pop();
            }
            event->msg.loop.track = track;
            event->msg.loop.count = loopCt;
            event->type = Evt::LOOPEND;
        }
        else
        {
            CLOG(LogLevel_t::Error, "unknown meta event " << (int)type);
        }
    }
    else
    {
        event->type = Evt::MIDI;
        if (status & 0x80) /* if high bit is set, then new status */
        {
            event->msg.midi.status = status;
            event->msg.midi.byte1 = getTrackByte(seq, track);
            seq->lastStatus[track] = status;
        }
        else /* running status */
        {
            if (seq->lastStatus[track] == 0)
            {
                THROW_RUNTIME_ERROR("Zero Running Status, unlikely a valid CSeq file");
            }

            event->msg.midi.status = seq->lastStatus[track];
            event->msg.midi.byte1 = status;
        }

        if (((event->msg.midi.status & 0xf0) != MIDI_ProgramChange) &&
            ((event->msg.midi.status & 0xf0) != MIDI_ChannelPressure))
        {
            event->msg.midi.byte2 = getTrackByte(seq, track);
            if ((event->msg.midi.status & 0xf0) == MIDI_NoteOn)
            {
                event->msg.midi.duration = readVarLen(seq, track);
            }
        }
        else
        {
            event->msg.midi.byte2 = 0;
        }
    }
    return true;
}

static float cSeqTicksToMSec(uint32_t division, uint32_t ticks, uint32_t tempo)
{
    return (ticks * tempo) / (division * 1000.0);
}

uint32_t N64CSeqWrapper::cSeqNextEvent(CSeqState *seq, CSeqEvent *evt, bool ignoreLoops)
{
    uint32_t i;
    uint32_t firstTime = 0xFFFFFFFF;
    uint32_t firstTrack = 0xFFFFFFFF;
    uint32_t lastTicks = seq->lastDeltaTicks;

    /* Warn if we are beyond the end of sequence. */
    if (!seq->validTracks)
    {
        CLOG(LogLevel_t::Warning, "Sequence Overrun");
    }

    for (i = 0; i < NMidiChannels; i++)
    {
        if ((seq->validTracks >> i) & 1)
        {
            if (seq->deltaFlag)
            {
                seq->evtDeltaTicks[i] -= lastTicks;
            }
            if (seq->evtDeltaTicks[i] < firstTime)
            {
                firstTime = seq->evtDeltaTicks[i];
                firstTrack = i;
            }
        }
    }

    if (0xFFFFFFFF == firstTrack)
    {
        THROW_RUNTIME_ERROR("should not happen");
    }

    evt->ticks = firstTime;
    this->cSeqGetTrackEvent(seq, firstTrack, evt, ignoreLoops);

    seq->lastTicks += firstTime;
    seq->lastDeltaTicks = firstTime;
    seq->lastMSec += cSeqTicksToMSec(seq->division, firstTime, seq->lastTempo);
    if (evt->type != Evt::TRACK_END && evt->type != Evt::SEQ_END)
    {
        seq->evtDeltaTicks[firstTrack] += readVarLen(seq, firstTrack);
    }
    seq->deltaFlag = true;

    return firstTrack;
}

/*
  Call this routine to handle the next event in the sequence.
  Assumes that the next sequence event is scheduled to be processed
  immediately since it does not check the event's tick time.

  sct 11/7/95
*/
void N64CSeqWrapper::handleNextSeqEvent(CSeqState *seq, CSeqEvent *evt, uint32_t track)
{
    switch (evt->type)
    {
        case Evt::MIDI:
            handleMIDIMsg(seq, evt, track);
            break;

        case Evt::TEMPO:
            handleMetaMsg(seq, evt, true);
            break;

        case Evt::SEQ_END:
            return;

        case Evt::LOOPEND:
        case Evt::TRACK_END:
        case Evt::LOOPSTART:
            break;
    }

    fluid_event_timer(this->evt, nullptr);
    this->synth->AddEvent(this->evt, seq->lastTicks);
}

// quickly go through all events to determine play duration
void N64CSeqWrapper::parseFirstTime(CSeqState *seq)
{
    CSeqEvent evt;
    bool cont = true;

    do
    {
        auto track = this->cSeqNextEvent(seq, &evt, true);

        switch (evt.type)
        {
            case Evt::TEMPO:
                handleMetaMsg(seq, &evt, false);
                break;

            case Evt::SEQ_END:
                cont = false;
                break;

            case Evt::LOOPSTART:
            case Evt::LOOPEND:
            {
                // zero-based
                auto loopId = evt.msg.loop.id;
                auto track = evt.msg.loop.track;
                auto &loops = this->trackLoops[track];

                if (evt.type == Evt::LOOPSTART)
                {
                    if (loops.size() <= loopId)
                    {
                        loops.resize(loopId + 1);
                    }

                    CSeqLoopInfo &info = loops[loopId];

                    info.trackId = track;
                    info.loopId = loopId;

                    if (!info.start.hasValue) // avoid multiple sets
                    {
                        info.start = seq->lastMSec;
                        info.start_tick = seq->lastTicks;
                    }
                }
                else
                {
                    if (loops.size() <= loopId)
                    {
                        CLOG(LogLevel_t::Warning, "Received loop end, but there was no corresponding loop start");
                    }
                    else
                    {
                        CSeqLoopInfo &info = loops[loopId];

                        if (!info.stop.hasValue)
                        {
                            info.stop = seq->lastMSec;
                            info.stop_tick = seq->lastTicks;
                            info.count = evt.msg.loop.count == 0xFF ? 0 : evt.msg.loop.count;
                        }
                    }
                }
            }
                [[fallthrough]];
            case Evt::TRACK_END:
            case Evt::MIDI:
            {
                MidiEvent *midi = &evt.msg.midi;
                int32_t status = midi->status & MIDI_StatusMask;
                uint8_t chan = midi->status & MIDI_ChannelMask;
                chan = track;

                auto vel = midi->byte2;
                switch (status)
                {
                    case MIDI_NoteOn:

                        if (vel != 0) /* a real note on */
                        {
                            this->synth->InformHasNoteOn(chan);
                        }

                        break;

                    case MIDI_ProgramChange:
                        this->synth->InformHasProgChange(chan);
                        break;
                }
            }
            break;

            default:
                CLOG(LogLevel_t::Error, "Ignoring unknown cseq event " << (int)evt.type);
                break;
        }
    } while (cont && cSeqNextSeqEvent(seq));
}

void N64CSeqWrapper::SequencerCallback(unsigned int /*time*/, fluid_event_t *e, fluid_sequencer_t * /*seq*/, void *data)
{
    N64CSeqWrapper *pthis = static_cast<N64CSeqWrapper *>(data);

    if (fluid_event_get_type(e) != FLUID_SEQ_TIMER)
    {
        return;
    }

    CSeqEvent evt;
    auto track = pthis->cSeqNextEvent(&pthis->seq, &evt, !pthis->lastUseLoopInfo);
    pthis->handleNextSeqEvent(&pthis->seq, &evt, track);
    cSeqNextSeqEvent(&pthis->seq);
}

static void alCSeqNew(CSeqState *seq, uint8_t *ptr, const std::string &filename)
{
    uint32_t i, tmpOff, flagTmp;

    seq->validTracks = 0;
    seq->lastDeltaTicks = 0;
    seq->lastTicks = 0;
    seq->deltaFlag = true;
    seq->lastTempo = 500000.0;
    seq->lastMSec = 0;

    std::string extension = ::getFileExtension(filename);

    uint32_t *bbase;
    uint32_t tracks = 16; // the default for N64 standard cmf format
    if (::iEquals(extension, "btmf"))
    {
        BTMidiHdr *base = (BTMidiHdr *)ptr;
        seq->division = be32toh(base->division);
        tracks = be32toh(base->totalTracks);
        bbase = (uint32_t *)ptr;
        bbase++;
        bbase++;
    }
    else //if(::iEquals(extension, "cmf"))
    {
        /* load the seqence pointed to by ptr   */
        CMidiHdr *base = (CMidiHdr *)ptr;
        seq->division = be32toh(base->division);
        bbase = (uint32_t *)ptr;
    }

    if (tracks > 32)
    {
        THROW_RUNTIME_ERROR("Sequence contains " << tracks << " tracks, but only 32 are supported.");
    }

    for (i = 0; i < tracks; i++)
    {
        seq->lastStatus[i] = 0;
        seq->curBckp[i] = 0;
        seq->curBckLen[i] = 0;
        seq->trackOffset[i] = tmpOff = be32toh(bbase[i]);
        if (tmpOff) /* if the track is valid */
        {
            flagTmp = 1 << i;
            seq->validTracks |= flagTmp;
            seq->cur[i] = (uint8_t *)((uintptr_t)ptr + tmpOff);
            seq->evtDeltaTicks[i] = readVarLen(seq, i);
        }
        else
        {
            seq->cur[i] = 0;
        }
    }
}

void N64CSeqWrapper::handleMIDIMsg(CSeqState *seq, CSeqEvent *event, uint32_t track)
{
    int32_t status;
    uint8_t chan;
    uint8_t key;
    uint8_t vel;
    uint8_t byte1;
    uint8_t byte2;
    MidiEvent *midi = &event->msg.midi;


    status = midi->status & MIDI_StatusMask;
    chan = midi->status & MIDI_ChannelMask;
    chan = track;
    byte1 = key = midi->byte1;
    byte2 = vel = midi->byte2;

    switch (status)
    {
        case MIDI_NoteOn:

            if (vel != 0) /* a real note on */
            {
                if (midi->duration)
                {
                    CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": NoteOn Event chan: " << (int)chan << " key: " << (int)key << " vel: " << (int)vel << " dur: " << (int)midi->duration);
                    fluid_event_note(this->evt, chan, key, vel, midi->duration);
                }
                else
                {
                    CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": NoteOn Event chan: " << (int)chan << " key: " << (int)key << " vel: " << (int)vel);
                    fluid_event_noteon(this->evt, chan, key, vel);
                }

                break;
            }

            /*
             * NOTE: intentional fall-through for note on with zero
             * velocity (Should never happen with compact midi sequence,
             * but could happen with real time midi.)
             */
            [[fallthrough]];

        case MIDI_NoteOff:
            CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": NoteOff Event chan: " << (int)chan << " key: " << (int)key);
            fluid_event_noteoff(this->evt, chan, key);
            break;

        case MIDI_PolyKeyPressure:
            CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": PolyKeyPressure Event chan: " << (int)chan << " key: " << (int)key << " pressure: " << (int)byte2);
            fluid_event_key_pressure(this->evt, chan, key, byte2);
            break;

        case MIDI_ChannelPressure:
            /*
             * Aftertouch per channel (hardwired to volume). Note that
             * aftertouch affects only notes that are already
             * sounding.
             */
            CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": ChannelPressure Event chan: " << (int)chan << " pressure: " << (int)byte1);
            fluid_event_channel_pressure(this->evt, chan, byte1);
            break;

        case MIDI_ControlChange:
            switch (byte1)
            {
                case MIDI_PAN_CTRL:
                    CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": Pan Event chan: " << (int)chan << " pan: " << (int)byte2);
                    fluid_event_pan(this->evt, chan, byte2);
                    break;

                case MIDI_VOLUME_CTRL:
                    CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": Volume Event chan: " << (int)chan << " vol: " << (int)byte2);
                    fluid_event_volume(this->evt, chan, byte2);
                    break;

                case MIDI_PRIORITY_CTRL:
                    // not supported
                    CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": Priority Event chan: " << (int)chan << " pri: " << (int)byte2);
                    break;

                case MIDI_SUSTAIN_CTRL:
                    CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": Sustain Event chan: " << (int)chan << " val: " << (int)byte2);
                    fluid_event_sustain(this->evt, chan, byte2);
                    break;

                case MIDI_FX1_CTRL:
                case MIDI_FX3_CTRL:
                default:
                    CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": CC Event chan: " << (int)chan << " CC: " << (int)byte1 << " val: " << (int)byte2);
                    fluid_event_control_change(this->evt, chan, byte1, byte2);
                    break;
            }
            break;

        case MIDI_ProgramChange:
            CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": ProgChange Event chan: " << (int)chan << " prog: " << (int)byte1);
            fluid_event_program_change(this->evt, chan, byte1);
            break;

        case MIDI_PitchBendChange:
        {
            int bendVal = ((byte2 << 7) | byte1);
            CLOG(LogLevel_t::Debug, "Tick " << seq->lastTicks << ": Pitch Event chan: " << (int)chan << " pitch: " << (int)bendVal);
            fluid_event_pitch_bend(this->evt, chan, bendVal);
        }
        break;

        default:
            CLOG(LogLevel_t::Error, "Unknown MIDI event: " << (int)status);
            break;
    }

    if (fluid_event_get_type(this->evt) == FLUID_SEQ_NOTE)
    {
        fluid_event_noteon(this->evt, chan, key, vel);
        this->synth->AddEvent(this->evt, seq->lastTicks);

        const unsigned int dur = fluid_event_get_duration(this->evt);
        fluid_event_noteoff(this->evt, chan, key);
        this->synth->AddEvent(this->evt, seq->lastTicks + dur);
    }
    else
    {
        this->synth->AddEvent(this->evt, seq->lastTicks);
    }
}

void N64CSeqWrapper::handleMetaMsg(CSeqState *seq, CSeqEvent *event, bool dispatchEvent)
{
    TempoEvent *tevt = &event->msg.tempo;
    int32_t tempo;

    if (event->msg.tempo.status == MIDI_Meta)
    {
        if (event->msg.tempo.type == MIDI_META_TEMPO)
        {
            tempo = (tevt->byte1 << 16) | (tevt->byte2 << 8) | (tevt->byte3 << 0);
            seq->lastTempo = tempo;

            if (dispatchEvent)
            {
                this->synth->ScheduleTempoChange(FluidsynthWrapper::GetTempoScale(tempo, seq->division), seq->lastTicks);
            }
        }
    }
}

void N64CSeqWrapper::open()
{
    this->evt = new_fluid_event();
    this->synth = new FluidsynthWrapper();
    this->synth->Init(::findSoundfont(this->Filename), this);

    this->Format.SampleRate = this->synth->GetSampleRate();

    if (gConfig.overridingGlobalLoopCount != this->lastOverridingLoopCount || gConfig.useLoopInfo != this->lastUseLoopInfo)
    {
        this->initAttr();
    }

    std::ifstream file(this->Filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    if (size > MaxFileSize)
    {
        THROW_RUNTIME_ERROR("CSeq file is bigger 1MiB. Unlikely an N64 sequence.");
    }

    file.seekg(0, std::ios::beg);

    this->fileBuf.resize(size);
    if (!file.read(reinterpret_cast<char *>(this->fileBuf.data()), size))
    {
        THROW_RUNTIME_ERROR("Unable to read file " << this->Filename);
    }

    alCSeqNew(&this->seq, this->fileBuf.data(), this->Filename);
    this->parseFirstTime(&this->seq);

    this->fileLen = std::ceil(this->seq.lastMSec);
    auto finishAtTick = this->seq.lastTicks;

    const auto *max = this->getLongestMidiTrackLoop();
    if (this->lastUseLoopInfo && max != nullptr)
    {
        this->fileLen.Value += (max->stop.Value - max->start.Value) * std::max(0, this->lastOverridingLoopCount - 1);
        finishAtTick += (max->stop_tick.Value - max->start_tick.Value) * std::max(0, this->lastOverridingLoopCount - 1);
    }

    // finally send note offs on all channels
    // we have to do this, because some wind might be blowing in DK64
    this->synth->FinishSong(finishAtTick);
    this->synth->ConfigureChannels(&this->Format);
    this->buildLoopTree();

    alCSeqNew(&this->seq, this->fileBuf.data(), this->Filename);

    this->synth->SetDefaultSeqTempoScale(this->seq.division);
    CSeqEvent evt;
    auto track = this->cSeqNextEvent(&this->seq, &evt, !this->lastUseLoopInfo);
    this->handleNextSeqEvent(&this->seq, &evt, track);
    cSeqNextSeqEvent(&this->seq);
}

const CSeqLoopInfo *N64CSeqWrapper::getLongestMidiTrackLoop() const
{
    const CSeqLoopInfo *max = nullptr;
    for (unsigned int t = 0; t < this->trackLoops.size(); t++)
    {
        for (unsigned int l = 0; l < this->trackLoops[t].size(); l++)
        {
            const CSeqLoopInfo &info = this->trackLoops[t][l];

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


void N64CSeqWrapper::close() noexcept
{
    if (this->synth != nullptr)
    {
        delete this->synth;
        this->synth = nullptr;
    }

    if (this->evt != nullptr)
    {
        delete_fluid_event(this->evt);
        this->evt = nullptr;
    }

    for (auto it = this->trackLoops.begin(); it != this->trackLoops.end(); ++it)
    {
        it->clear();
        it->shrink_to_fit();
    }

    this->fileBuf.clear();
    this->fileBuf.shrink_to_fit();
}

frame_t N64CSeqWrapper::getFrames() const
{
    size_t len = this->fileLen.Value;
    len += 3000;
    //     if(gConfig.FluidsynthEnableReverb)
    //     {
    //         len += (gConfig.FluidsynthRoomSize*10.0 * gConfig.FluidsynthLevel * 1000) / 2;
    //     }

    return (this->Format.Voices == 0) ? 0 : msToFrames(len, this->Format.SampleRate);
}

vector<loop_t> N64CSeqWrapper::getLoopArray() const noexcept
{
    vector<loop_t> loopArr;
    // if the loop should be unrolled and loop info is respected, do not attemtp to search for a potential master loop and simply unroll all midi track loops
    //
    // if the loop info is used but the song is not rendered in a single buffer AND the user has requested infinite playback, also unroll everything "forever"
    if (gConfig.useLoopInfo &&
        (this->lastOverridingLoopCount >= 1 ||
         (this->lastOverridingLoopCount <= 0 && !gConfig.RenderWholeSong)))
    {
        return loopArr;
    }

    // so, here we are, having a bunch of individually looped midi tracks (maybe)
    // somehow trying to put those loops together so that we find a valid master loop within the generated PCM

    const auto *max = this->getLongestMidiTrackLoop();
    if (max != nullptr)
    {
        loop_t l;
        l.start = ::msToFrames(max->start.Value, this->Format.SampleRate);
        l.stop = ::msToFrames(max->stop.Value, this->Format.SampleRate);
        l.count = max->count;

        loopArr.push_back(l);
    }
    return loopArr;
}

void N64CSeqWrapper::render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender)
{
    STANDARDWRAPPER_RENDER(float, this->synth->Render(pcm, framesToDoNow))

    this->doAudioNormalization(static_cast<float *>(bufferToFill), framesToRender);
}
