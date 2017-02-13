#pragma once

#include "IAudioOutput.h"
#include "Song.h"
#include "CommonExceptions.h"
#include <cstdio>
#include <mutex>

typedef int32_t chunk_size_t;
union chunk_element_t
{
    uint32_t uDword = 0;
     int32_t sDword;
    uint16_t Word[2];
    char Byte[4];
    
    chunk_element_t(uint32_t val) : uDword(val) {}
    chunk_element_t( int32_t val) : sDword(val) {}
    chunk_element_t(uint16_t val0, uint16_t val1)
    {
        this->Word[0] = val0;
        this->Word[1] = val1;
    }
    chunk_element_t(const char val[4])
    {
            this->Byte[0] = val[0];
            this->Byte[1] = val[1];
            this->Byte[2] = val[2];
            this->Byte[3] = val[3];
    }
};

struct sample_loop_t
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

struct WaveHeader
{
public:
    vector<chunk_element_t> data;
    
private:
    const char RiffID[4] = {'R','I','F','F'};
    chunk_size_t RiffSize;
    const char WaveID[4] = {'W','A','V','E'};

    /*******************************************
     *  Sampler CHUNK (for loop info)
     ******************************************/
    const char SamplerID[4] = {'s','m','p','l'};
    const chunk_size_t SamplerSize = sizeof(WaveHeader::Manufacturer) + sizeof(WaveHeader::Product) + sizeof(WaveHeader::SamplePeriod) + sizeof(WaveHeader::MidiRootNote) + sizeof(WaveHeader::MidiPitchFraction) + sizeof(WaveHeader::SMPTEFormat) + sizeof(WaveHeader::SMPTEOffset) + sizeof(WaveHeader::SampleLoops) + sizeof(WaveHeader::SamplerData) /*+ sizeof(WaveHeader::loops)*/;
    const int32_t Manufacturer = 0; // not intended for specific manufacturer
    const int32_t Product = 0; // not intended for specific manufacturer's product
    uint32_t SamplePeriod; // == (1.0/SampleRate)/(1e-9)
    const uint32_t MidiRootNote = 60; // dont care, use middle C
    const uint32_t MidiPitchFraction = 0; // no fine tuning;
    const uint32_t SMPTEFormat = 0; // no SMPTE offset
    const uint32_t SMPTEOffset = 0; // no SMPTE offset
          uint32_t SampleLoops;
    const uint32_t SamplerData = 0; // no additional info following this chunk

    /*******************************************
     *  List CHUNK (for metadata)
     ******************************************/
    const char ListID[4] = {'L','I','S','T'};
    const chunk_size_t ListSize = sizeof(WaveHeader::Manufacturer) + sizeof(WaveHeader::Product) + sizeof(WaveHeader::SamplePeriod) + sizeof(WaveHeader::MidiRootNote) + sizeof(WaveHeader::MidiPitchFraction) + sizeof(WaveHeader::SMPTEFormat) + sizeof(WaveHeader::SMPTEOffset) + sizeof(WaveHeader::SampleLoops) + sizeof(WaveHeader::SamplerData) /*+ sizeof(WaveHeader::loops)*/;
    const char ListType[4] = {'I','N','F','O'}; // this list chunk is of type INFO

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
        
        
        // the outer most RIFF chunk
        data.push_back(chunk_element_t(RiffID));
        data.push_back(RiffSize);
        data.push_back(WaveID);
        
        vector<loop_t> loops = s->getLoopArray();
        if(loops.size())
        {
            // smpl chunk
            data.push_back(SamplerID);
            data.push_back(SamplerSize);
            data.push_back(Manufacturer);
            data.push_back(Product);
            data.push_back(SamplePeriod);
            data.push_back(MidiRootNote);
            data.push_back(MidiPitchFraction);
            data.push_back(SMPTEFormat);
            data.push_back(SMPTEOffset);
            
            SampleLoops = loops.size();
            data.push_back(SampleLoops);
            data.push_back(SamplerData);
            
//             struct SampleLoop loops
        }
        
        // LIST chunk - metadata
        data.push_back(ListID);
        data.push_back(ListSize);
        data.push_back(ListType);
        
        bool atLeastOne = false;
        const SongInfo& m = s->Metadata;
        {
#define WRITE_META(a,b,c,d) \
            if(!meta.empty())\
            {\
                constexpr char ID[4] = {a,b,c,d};\
                atLeastOne |= true;\
                \
                /* require that meta.size() % 4 == 0 */\
                size_t padd = sizeof(chunk_element_t) - (meta.size() & sizeof(chunk_element_t));\
                for(size_t i=0; i<padd; i++)\
                {\
                    meta.push_back('\0');\
                }\
                \
                data.push_back(ID);\
                uint32_t size = meta.size();\
                data.push_back(size);\
                for(size_t i=0; i<size; i+=4)\
                {\
                    const char dings[4] = {meta[i], meta[i+1], meta[i+2], meta[i+3]};\
                    data.push_back(dings);\
                }\
            }
            
        string meta = m.Artist;
        WRITE_META('I','A','R','T');
        
        meta = m.Genre;
        WRITE_META('I','G','N','R');
        
        meta = m.Title;
        WRITE_META('I','N','A','M');
        
        meta = m.Track;
        WRITE_META('I','T','R','K');
        
        meta = m.Comment;
        WRITE_META('I','C','M','T');
        
        meta = m.Year;
        WRITE_META('I','C','R','D');
        
        meta = m.Album;
        WRITE_META('I','P','R','D');
        
#undef WRITE_META
        }
        
        if(!atLeastOne)
        {
            // not a single metadata set? well then, remove the list chunk
            data.pop_back();
            data.pop_back();
            data.pop_back();
        }
        
        
        // fmt chunk
        data.push_back(FormatID);
        data.push_back(FormatSize);
        data.push_back(FormatType);
        data.push_back(Channels);
        data.push_back(SampleRate);
        data.push_back(BytesPerSecond);
        data.push_back(BlockAlign);
        data.push_back(BitsPerSample);
        
        // start the data chunk
        data.push_back(DataID);
        data.push_back(DataSize);
    }
};


class Player;

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

    // interface methods declaration

    void open () override;

    void init (SongFormat format, bool realtime = false) override;

    void close () override;

    int write (const float* buffer, frame_t frames) override;

    int write (const int16_t* buffer, frame_t frames) override;

    int write (const int32_t* buffer, frame_t frames) override;

    void start () override;
    void stop () override;

private:
    // because this.stop() might be called concurrently to this.write()
    mutable mutex mtx;
    
    Player* player = nullptr;
    FILE* handle = nullptr;

    const Song* currentSong = nullptr;
    frame_t framesWritten = 0;

    template<typename T> int write(const T* buffer, frame_t frames);
};
