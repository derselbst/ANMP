#include "WaveOutput.h"
#include "Common.h"
#include "Config.h"
#include "Player.h"
#include "Song.h"

#include "AtomicWrite.h"
#include "CommonExceptions.h"
#include "LoudnessFile.h"
#include "StringFormatter.h"


#include <cstring>
#include <errno.h>
#include <iostream>
#include <string>
#include <utility> // std::pair

typedef int32_t chunk_size_t;
union chunk_element_t
{
    uint32_t uDword = 0;
    int32_t sDword;
    uint16_t Word[2];
    char Byte[4];

    chunk_element_t(uint32_t val)
    : uDword(val)
    {
    }
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
    const char RiffID[4] = {'R', 'I', 'F', 'F'};
    chunk_size_t RiffSize = 0; // calculated at the very end
    const char WaveID[4] = {'W', 'A', 'V', 'E'};

    /*******************************************
     *  FORMAT CHUNK (must precede data chunk)
     ******************************************/
    const char FormatID[4] = {'f', 'm', 't', ' '};
    uint16_t FormatType;
    uint16_t Channels;
    uint32_t SampleRate;
    uint32_t BytesPerSecond; // == SampleRate*BlockAlign
    uint16_t BlockAlign; // == (BitsPerSample * Channels) / 8
    uint16_t BitsPerSample;
    static constexpr chunk_size_t FormatSize = sizeof(FormatType) + sizeof(Channels) + sizeof(SampleRate) + sizeof(BytesPerSecond) + sizeof(BlockAlign) + sizeof(BitsPerSample); // all in all 16

    /*******************************************
     *  DATA CHUNK
     ******************************************/
    const char DataID[4] = {'d', 'a', 't', 'a'};
    chunk_size_t DataSize; // == Frames*Channels*(BitsPerSample/8)

    /*******************************************
     *  Sampler CHUNK (for loop info)
     ******************************************/
    const char SamplerID[4] = {'s', 'm', 'p', 'l'};
    chunk_size_t SamplerSize = sizeof(Manufacturer) + sizeof(Product) + sizeof(SamplePeriod) + sizeof(MidiRootNote) + sizeof(MidiPitchFraction) + sizeof(SMPTEFormat) + sizeof(SMPTEOffset) + sizeof(SampleLoops) + sizeof(SamplerData) /*+ sizeof(loops)*/;
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
    const char ListID[4] = {'L', 'I', 'S', 'T'};
    chunk_size_t ListSize = 0; // calculated during writing metadata
    const char ListType[4] = {'I', 'N', 'F', 'O'}; // this list chunk is of type INFO

    public:
    // in file offset, where to start writing pcm
    static constexpr chunk_size_t DataOffset = sizeof(RiffID) + sizeof(RiffSize) + sizeof(WaveID) + sizeof(FormatID) + sizeof(FormatSize) + FormatSize + sizeof(DataID) + sizeof(DataSize); // == 44

    // contains the riff, fmt and data chunks, i.e. fixed length
    std::vector<chunk_element_t> header;
    // contains list and smpl chunks, i.e. might be there, or not
    std::vector<chunk_element_t> meta;

    WaveHeader(const Song *s, const int framesWritten)
    {
        this->Channels = s->Format.Channels();
        this->SampleRate = s->Format.SampleRate;

        switch (s->Format.SampleFormat)
        {
            case SampleFormat_t::float32:
                this->FormatType = FMT_IEEE_FLOAT;
                this->BitsPerSample = 32;
                break;
            case SampleFormat_t::int16:
                this->FormatType = FMT_PCM;
                this->BitsPerSample = 16;
                break;
            case SampleFormat_t::int32:
                this->FormatType = FMT_PCM;
                this->BitsPerSample = 32;
                break;

            case SampleFormat_t::unknown:
                throw invalid_argument("pcmFormat mustnt be unknown");
                break;

            default:
                throw NotImplementedException();
                break;
        }

        /// calculations following

        this->DataSize = framesWritten * this->Channels * (this->BitsPerSample / 8);
        this->BlockAlign = this->BitsPerSample * this->Channels / 8;
        this->BytesPerSecond = this->SampleRate * this->BlockAlign;
        this->SamplePeriod = (1.0 / this->SampleRate) / (1e-9);

        /// end calcs

        // the outer most RIFF chunk
        this->header.push_back(chunk_element_t(RiffID));

        this->header.push_back(RiffSize);
        size_t RiffSizeIdx = this->header.size() - 1; // remember where size of riff chunk is stored, correct it later

        this->header.push_back(WaveID);

        // fmt chunk
        this->header.push_back(FormatID);
        this->header.push_back(FormatSize);
        this->header.push_back(chunk_element_t(FormatType, Channels));
        this->header.push_back(SampleRate);
        this->header.push_back(BytesPerSecond);
        this->header.push_back(chunk_element_t(BlockAlign, BitsPerSample));

        // start the data chunk
        this->header.push_back(DataID);
        this->header.push_back(DataSize);


        const std::vector<loop_t> &loops = s->getLoopArray();
        if ((SampleLoops = loops.size()) > 0)
        {
            // smpl chunk
            this->meta.push_back(SamplerID);
            SamplerSize += SampleLoops * 24; // sizeof each loop array embedded in the file
            this->meta.push_back(SamplerSize);
            this->meta.push_back(Manufacturer);
            this->meta.push_back(Product);
            this->meta.push_back(SamplePeriod);
            this->meta.push_back(MidiRootNote);
            this->meta.push_back(MidiPitchFraction);
            this->meta.push_back(SMPTEFormat);
            this->meta.push_back(SMPTEOffset);

            this->meta.push_back(SampleLoops);
            this->meta.push_back(SamplerData);

            for (size_t i = 0; i < SampleLoops; i++)
            {
                this->meta.push_back(i); // unique loop identifier
                this->meta.push_back(0U); // forward loop;
                this->meta.push_back(loops[i].start);
                this->meta.push_back(loops[i].stop);
                this->meta.push_back(0U); // no fine loop adjustment
                this->meta.push_back(loops[i].count);
            }
        }

        // LIST chunk - metadata
        this->meta.push_back(ListID);

        this->meta.push_back(chunk_element_t(ListSize + sizeof(ListType)));
        size_t ListSizeIdx = this->meta.size() - 1;

        this->meta.push_back(ListType);

        bool atLeastOne = false;
        const SongInfo &m = s->Metadata;
        {
#define WRITE_META(a, b, c, d)                                                               \
    if (!meta.empty())                                                                       \
    {                                                                                        \
        constexpr char ID[4] = {a, b, c, d};                                                 \
        atLeastOne |= true;                                                                  \
                                                                                             \
        if (meta.size() % 4 != 0)                                                            \
        {                                                                                    \
            size_t padd = sizeof(chunk_element_t) - (meta.size() % sizeof(chunk_element_t)); \
            for (size_t i = 0; i < padd; i++)                                                \
            {                                                                                \
                meta.push_back('\0');                                                        \
            }                                                                                \
        }                                                                                    \
                                                                                             \
        this->meta.push_back(ID);                                                            \
        uint32_t size = meta.size();                                                         \
        this->meta[ListSizeIdx].uDword += sizeof(ID) + sizeof(size) + size;                  \
        this->meta.push_back(size);                                                          \
        for (size_t i = 0; i < size; i += 4)                                                 \
        {                                                                                    \
            const char dings[4] = {meta[i], meta[i + 1], meta[i + 2], meta[i + 3]};          \
            this->meta.push_back(dings);                                                     \
        }                                                                                    \
    }

            std::string meta = m.Artist;
            WRITE_META('I', 'A', 'R', 'T');

            meta = m.Genre;
            WRITE_META('I', 'G', 'N', 'R');

            meta = m.Title;
            WRITE_META('I', 'N', 'A', 'M');

            meta = m.Track;
            WRITE_META('I', 'T', 'R', 'K');

            meta = m.Comment;
            WRITE_META('I', 'C', 'M', 'T');

            meta = m.Year;
            WRITE_META('I', 'C', 'R', 'D');

            meta = m.Album;
            WRITE_META('I', 'P', 'R', 'D');

#undef WRITE_META
        }

        if (!atLeastOne)
        {
            // not a single metadata set? well then, remove the list chunk
            this->meta.pop_back();
            this->meta.pop_back();
            this->meta.pop_back();
        }

        this->RiffSize = this->header.size() + this->meta.size();
        // add no. of bytes that are in data chunk
        this->RiffSize += DataSize;
        // the 8 bytes of RIFF chunk dont count
        this->RiffSize -= (sizeof(RiffID) + sizeof(RiffSize));
        this->header[RiffSizeIdx].uDword = this->RiffSize;
    }
};

WaveOutput::WaveOutput(Player *player)
: player(player)
{
    this->player->onCurrentSongChanged += make_pair(this, WaveOutput::onCurrentSongChanged);
}

WaveOutput::~WaveOutput()
{
    this->close();
    this->player->onCurrentSongChanged -= this;
}

//
// Interface Implementation
//
void WaveOutput::open()
{
    lock_guard<recursive_mutex> lck(this->mtx);

    if (gConfig.RenderWholeSong && gConfig.PreRenderTime != 0)
    {
        // writing the file might be done with one call to this->write(), but this doesnt mean that the song already has been fully rendered yet
        THROW_RUNTIME_ERROR("You MUST NOT hold the whole audio file in memory, when using WaveOutput, unless you set gConfig.PreRenderTime==0")
    }
}

void WaveOutput::init(SongFormat &format, bool)
{
    this->currentFormat = format;
}

void WaveOutput::onCurrentSongChanged(void *context, const Song *newSong)
{
    WaveOutput *pthis = static_cast<WaveOutput *>(context);

    lock_guard<recursive_mutex> lck(pthis->mtx);
    try
    {
        pthis->close();

        if (newSong != nullptr)
        {
            std::string outFile = StringFormatter::Singleton().GetFilename(newSong, ".wav");

            pthis->handle = fopen(outFile.c_str(), "wb");
            if (pthis->handle == nullptr)
            {
                THROW_RUNTIME_ERROR("failed opening \"" << outFile << "\" for writing")
            }

            fseek(pthis->handle, WaveHeader::DataOffset, SEEK_SET);
        }
    }
    catch (...)
    {
        pthis->currentSong = nullptr;
        throw;
    }

    pthis->currentSong = newSong;
}

void WaveOutput::close()
{
    lock_guard<recursive_mutex> lck(this->mtx);

    if (this->handle != nullptr && this->currentSong != nullptr && this->framesWritten > 0)
    {
        WaveHeader w(this->currentSong, this->framesWritten);

        fwrite(w.meta.data(), w.meta.size(), sizeof(decltype(w.meta)::value_type), this->handle);

        fseek(this->handle, 0, SEEK_SET);
        fwrite(w.header.data(), w.header.size(), sizeof(decltype(w.header)::value_type), this->handle);
        fclose(this->handle);
        this->handle = nullptr;
    }

    this->currentSong = nullptr;
    this->framesWritten = 0;
}


template<typename T>
int WaveOutput::write(const T *buffer, frame_t frames)
{
    int ret = 0;

    lock_guard<recursive_mutex> lck(this->mtx);

    if (this->handle != nullptr)
    {
        if (!this->currentFormat.IsValid())
        {
            CLOG(LogLevel_t::Warning, "attempting to use invalid SongFormat");
        }

        uint32_t chan = this->currentFormat.Channels();
        ret = fwrite(buffer, sizeof(T), frames * chan, this->handle);
        ret /= chan;

        if (ret != frames)
        {
            THROW_RUNTIME_ERROR("fwrite failed writing " << frames << " frames, errno says: " << strerror(errno))
        }

        this->framesWritten += ret;
    }

    return ret;
}

int WaveOutput::write(const float *buffer, frame_t frames)
{
    return this->write<float>(buffer, frames);
}

int WaveOutput::write(const int16_t *buffer, frame_t frames)
{
    return this->write<int16_t>(buffer, frames);
}

int WaveOutput::write(const int32_t *buffer, frame_t frames)
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
