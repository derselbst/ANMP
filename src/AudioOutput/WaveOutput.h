#pragma once

#include "IAudioOutput.h"
#include "Song.h"
#include "CommonExceptions.h"
#include <cstdio>

typedef int32_t chunk_size_t;

class Player;

struct SampleLoop
{
  uint32_t Identifier; // some unique number
  uint32_t LoopType;
  uint32_t Start; // frame offset
  uint32_t End; // frame offset, will also be played
  static const uint32_t Fraction = 0; // no fine loop adjustment
  uint32_t PlayCount;
};

#define FMT_PCM 0x0001
#define FMT_IEEE_FLOAT 0x0003

constexpr int N = 1;
struct WaveHeader
{
    const char RiffID[4] = {'R','I','F','F'};
    chunk_size_t RiffSize;
    const char WaveID[4] = {'W','A','V','E'};
    
    
    /*******************************************
     *  Sampler CHUNK (for loop info)
     ******************************************/
    const char SamplerID[4] = {'s','m','p','l'};
    const chunk_size_t SamplerSize = sizeof(WaveHeader::Manufacturer) + sizeof(WaveHeader::Product) + sizeof(WaveHeader::SamplePeriod) + sizeof(WaveHeader::MidiRootNote) + sizeof(WaveHeader::MidiPitchFraction) + sizeof(WaveHeader::SMPTEFormat) + sizeof(WaveHeader::SMPTEOffset) + sizeof(WaveHeader::SampleLoops) + sizeof(WaveHeader::SamplerData) + sizeof(WaveHeader::loops);
    const int32_t Manufacturer = 0; // not intended for specific manufacturer
    const int32_t Product = 0; // not intended for specific manufacturer's product
    uint32_t SamplePeriod; // == (1.0/SampleRate)/(1e-9)
    const uint32_t MidiRootNote = 60; // dont care, use middle C
    const uint32_t MidiPitchFraction = 0; // no fine tuning;
    const uint32_t SMPTEFormat = 0; // no SMPTE offset
    const uint32_t SMPTEOffset = 0; // no SMPTE offset
    const uint32_t SampleLoops = N;
    const uint32_t SamplerData = 0; // no additional info following this chunk
    struct SampleLoop loops[N];
    
    
    /*******************************************
     *  FORMAT CHUNK (must precede data chunk)
     ******************************************/
    const char FormatID[4] = {'f','m','t',' '};
    const chunk_size_t FormatSize = sizeof(WaveHeader::FormatType) + sizeof(WaveHeader::Channels) + sizeof(WaveHeader::SampleRate) + sizeof(WaveHeader::BytesPerSecond) + sizeof(WaveHeader::BlockAlign) + sizeof(WaveHeader::BitsPerSample) /*==16*/;
    uint16_t FormatType;
    uint16_t Channels;
    uint32_t SampleRate;
    uint32_t BytesPerSecond; // == SampleRate*BlockAlign
    uint16_t BlockAlign; // == (BitsPerSample * Channels) / 8
    uint16_t BitsPerSample;
    
    /*******************************************
     *  DATA CHUNK
     ******************************************/
    const char DataID[4] = {'d','a','t','a'};
    chunk_size_t DataSize; // == Frames*Channels*(BitsPerSample/8)
    
    WaveHeader(const Song* s)
    {
        this->Channels = s->Format.Channels;
        this->SampleRate = s->Format.SampleRate;
        
        switch (s->Format.SampleFormat)
        {
        case float32:
            this->FormatType = FMT_IEEE_FLOAT;
            this->BitsPerSample = 32;
            break;
        case int16:
            this->FormatType = FMT_PCM;
            this->BitsPerSample = 16;
            break;
        case int32:
            this->FormatType = FMT_PCM;
            this->BitsPerSample = 32;
            break;
            
        case unknown:
            throw invalid_argument("pcmFormat mustnt be unknown");
            break;

        default:
            throw NotImplementedException();
            break;
        }
        
        /// calculations following
        
        this->DataSize = s->getFrames() * s->Format.Channels * (this->BitsPerSample/8);
        this->BlockAlign = this->BitsPerSample * this->Channels / 8;
        this->BytesPerSecond = this->SampleRate * this->BlockAlign;
        this->SamplePeriod = (1.0/this->SampleRate)/(1e-9);
        
        /// end calcs
        
        this->RiffSize = sizeof(*this);
        // the 8 bytes of RIFF chunk dont count
        this->RiffSize -= (sizeof(RiffID) + sizeof(RiffSize));
        // add no. of bytes that are in data chunk
        this->RiffSize += DataSize;
    }
};

/**
  * class WaveOutput
  *
  * A wrapper library for ALSA, enabling ANMP to use ALSA for playback
  */
class WaveOutput : public IAudioOutput
{
public:

    WaveOutput (Player*);

    // forbid copying
    WaveOutput(WaveOutput const&) = delete;
    WaveOutput& operator=(WaveOutput const&) = delete;
    
    virtual ~WaveOutput ();

    static void SongChanged(void* ctx);
    
    
    // interface methods declaration

    void open () override;

    void init (unsigned int sampleRate, uint8_t channels, SampleFormat_t s, bool realtime = false) override;

    void close () override;

    int write (float* buffer, frame_t frames) override;

    int write (int16_t* buffer, frame_t frames) override;

    int write (int32_t* buffer, frame_t frames) override;

    void setVolume(uint8_t) override;

    void start () override;
    void stop () override;

private:
    Player* player = nullptr;
    FILE* handle = nullptr;
    
    const Song* currentSong = nullptr;
    frame_t framesWritten = 0;

};
