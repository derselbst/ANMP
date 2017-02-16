#include "WaveOutput.h"
#include "Player.h"
#include "Song.h"
#include "Config.h"
#include "Common.h"

#include "LoudnessFile.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <iostream>
#include <string>
#include <utility>      // std::pair
#include <cstring>
#include <errno.h>

typedef int32_t chunk_size_t;
union chunk_element_t
{
    uint32_t uDword = 0;
     int32_t sDword;
    uint16_t Word[2];
    char Byte[4];
    
    chunk_element_t(uint32_t val) : uDword(val) {}
//     chunk_element_t( int32_t val) : sDword(val) {}
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

#define FMT_PCM 0x0001
#define FMT_IEEE_FLOAT 0x0003

struct WaveHeader
{
private:
    const char RiffID[4] = {'R','I','F','F'};
    chunk_size_t RiffSize = 0; // calculated at the very end
    const char WaveID[4] = {'W','A','V','E'};

    /*******************************************
     *  Sampler CHUNK (for loop info)
     ******************************************/
    const char SamplerID[4] = {'s','m','p','l'};
    chunk_size_t SamplerSize = sizeof(WaveHeader::Manufacturer) + sizeof(WaveHeader::Product) + sizeof(WaveHeader::SamplePeriod) + sizeof(WaveHeader::MidiRootNote) + sizeof(WaveHeader::MidiPitchFraction) + sizeof(WaveHeader::SMPTEFormat) + sizeof(WaveHeader::SMPTEOffset) + sizeof(WaveHeader::SampleLoops) + sizeof(WaveHeader::SamplerData) /*+ sizeof(WaveHeader::loops)*/;
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
    chunk_size_t ListSize = 0; // calculated during writing metadata
    const char ListType[4] = {'I','N','F','O'}; // this list chunk is of type INFO

    /*******************************************
     *  FORMAT CHUNK (must precede data chunk)
     ******************************************/
    const char FormatID[4] = {'f','m','t',' '};
    const chunk_size_t FormatSize = sizeof(WaveHeader::FormatType) + sizeof(WaveHeader::Channels) + sizeof(WaveHeader::SampleRate) + sizeof(WaveHeader::BytesPerSecond) + sizeof(WaveHeader::BlockAlign) + sizeof(WaveHeader::BitsPerSample); // all in all 16
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

public:
    vector<chunk_element_t> data;
    
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

        
        
        // the outer most RIFF chunk
        data.push_back(chunk_element_t(RiffID));
        
        data.push_back(RiffSize);
        size_t RiffSizeIdx = data.size()-1; // remember where size of riff chunk is stored, correct it later
        
        data.push_back(WaveID);
        
        vector<loop_t> loops = s->getLoopArray();
        if((SampleLoops = loops.size()) > 0)
        {
            // smpl chunk
            data.push_back(SamplerID);
            SamplerSize += SampleLoops * 24; // sizeof each loop array embedded in the file
            data.push_back(SamplerSize);
            data.push_back(Manufacturer);
            data.push_back(Product);
            data.push_back(SamplePeriod);
            data.push_back(MidiRootNote);
            data.push_back(MidiPitchFraction);
            data.push_back(SMPTEFormat);
            data.push_back(SMPTEOffset);
            
            data.push_back(SampleLoops);
            data.push_back(SamplerData);

            for(size_t i=0; i<SampleLoops; i++)
            {
                data.push_back(i); // unique loop identifier
                data.push_back(0U); // forward loop;
                data.push_back(loops[i].start);
                data.push_back(loops[i].stop);
                data.push_back(0U); // no fine loop adjustment
                data.push_back(loops[i].count);
            }
        }
        
        // LIST chunk - metadata
        data.push_back(ListID);
        
        data.push_back(chunk_element_t(ListSize + sizeof(ListType)));
        size_t ListSizeIdx = data.size()-1;
        
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
                if(meta.size() % 4 != 0)\
                {\
                    size_t padd = sizeof(chunk_element_t) - (meta.size() % sizeof(chunk_element_t));\
                    for(size_t i=0; i<padd; i++)\
                    {\
                        meta.push_back('\0');\
                    }\
                }\
                \
                data.push_back(ID);\
                uint32_t size = meta.size();\
                data[ListSizeIdx].uDword += sizeof(ID) + sizeof(size) + size;\
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
        data.push_back(chunk_element_t(FormatType, Channels));
        data.push_back(SampleRate);
        data.push_back(BytesPerSecond);
        data.push_back(chunk_element_t(BlockAlign, BitsPerSample));
        
        // start the data chunk
        data.push_back(DataID);
        data.push_back(DataSize);
                
        
        this->RiffSize = this->data.size();
        // add no. of bytes that are in data chunk
        this->RiffSize += DataSize;
        // the 8 bytes of RIFF chunk dont count
        this->RiffSize -= (sizeof(RiffID) + sizeof(RiffSize));
        data[RiffSizeIdx].uDword = this->RiffSize;
    }
};

WaveOutput::WaveOutput(Player* player):player(player)
{
}

WaveOutput::~WaveOutput()
{
    this->close();
}

//
// Interface Implementation
//
void WaveOutput::open()
{
    lock_guard<mutex> lck(this->mtx);
    
    if(gConfig.RenderWholeSong && gConfig.PreRenderTime!=0)
    {
        // writing the file might be done with one call to this->write(), but this doesnt mean that the song already has been fully rendered yet
        THROW_RUNTIME_ERROR("You MUST NOT hold the whole audio file in memory, when using WaveOutput, while gConfig.PreRenderTime!=0")
    }
}

void WaveOutput::init(SongFormat format, bool)
{
    this->close();
    
    lock_guard<mutex> lck(this->mtx);
    
    this->currentSong = this->player->getCurrentSong();
    
    if(this->currentSong == nullptr || !format.IsValid())
    {
        return;
    }

    string outFile = ::getUniqueFilename(this->currentSong->Filename + ".wav");
    
    this->handle = fopen(outFile.c_str(), "wb");
    if(this->handle == nullptr)
    {
        THROW_RUNTIME_ERROR("failed opening \"" << outFile << "\" for writing")
    }

    fseek(this->handle, sizeof(WaveHeader), SEEK_SET);
    
    this->currentFormat = format;
}

void WaveOutput::close()
{
    lock_guard<mutex> lck(this->mtx);
    
    if(this->handle != nullptr && this->currentSong != nullptr)
    {
        WaveHeader w(this->currentSong);

        fseek(this->handle, 0, SEEK_SET);
        fwrite(w.data.data(), w.data.size(), sizeof(decltype(w.data)::value_type), this->handle);
        fclose(this->handle);
        this->handle = nullptr;
    }

    this->currentSong = nullptr;
    this->framesWritten = 0;
}


template<typename T> int WaveOutput::write(const T* buffer, frame_t frames)
{
    int ret=0;
    
    lock_guard<mutex> lck(this->mtx);
    
    if(this->handle!=nullptr)
    {
        ret = fwrite(buffer, sizeof(T), frames * this->currentFormat.Channels, this->handle);
        ret /= this->currentFormat.Channels;
        
        if(ret != frames)
        {
            THROW_RUNTIME_ERROR("fwrite failed writing " << frames << " frames, errno says: " << strerror(errno))
        }

        this->framesWritten += ret;
    }
    
    return ret;
}

int WaveOutput::write (const float* buffer, frame_t frames)
{    
    return this->write<float>(buffer, frames);
}

int WaveOutput::write (const int16_t* buffer, frame_t frames)
{    
    return this->write<int16_t>(buffer, frames);
}

int WaveOutput::write (const int32_t* buffer, frame_t frames)
{
    return this->write<int32_t>(buffer, frames);
}

void WaveOutput::start()
{

}

void WaveOutput::stop()
{
    this->close();
}

