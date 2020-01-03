#include "LibSNDWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Config.h"


#include <chrono>
#include <cstring>
#include <thread> // std::this_thread::sleep_for
#include <utility>
#include <fstream>
#include <regex>


LibSNDWrapper::LibSNDWrapper(string filename)
: StandardWrapper(std::move(filename))
{
    this->init();
}

LibSNDWrapper::LibSNDWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len)
: StandardWrapper(std::move(filename), offset, len)
{
    this->init();
}

void LibSNDWrapper::init()
{
    std::ifstream ifs(this->Filename, ios::binary);
    std::array<char, 1<<14> buf;
    ifs.read(buf.data(), sizeof(buf));
    std::string str(buf.data(), ifs.gcount());
    std::regex rxStart("LOOPSTART=([0-9]+)");
    std::regex rxLen("LOOPLENGTH=([0-9]+)");

    auto lstart_begin = std::sregex_iterator(str.begin(), str.end(), rxStart);
    auto llength_begin = std::sregex_iterator(str.begin(), str.end(), rxLen);
    auto words_end = std::sregex_iterator();

    if(lstart_begin != words_end && llength_begin != words_end)
    {
        std::smatch start_match = *lstart_begin, len_match = *llength_begin;
        if(start_match.size() == 2 && len_match.size() == 2)
        {
            this->legacyLoop.start = std::stoi(start_match[1].str());
            this->legacyLoop.stop = this->legacyLoop.start + std::stoi(len_match[1].str());
        }
    }
}

LibSNDWrapper::~LibSNDWrapper()
{
    this->releaseBuffer();
    this->close();
}


void LibSNDWrapper::open()
{
    // avoid multiple calls to open()
    if (this->sndfile != nullptr)
    {
        return;
    }

    memset(&sfinfo, 0, sizeof(sfinfo));
    if ((this->sndfile = sf_open(this->Filename.c_str(), SFM_READ, &sfinfo)) == nullptr)
    {
        THROW_RUNTIME_ERROR(sf_strerror(nullptr) << " (in File \"" << this->Filename << ")\"");
    };

    if (sfinfo.channels < 1)
    {
        THROW_RUNTIME_ERROR("channels == " << sfinfo.channels);
    };

    // group all available channels to individual stereo voices
    this->Format.ConfigureVoices(sfinfo.channels, 2);
    this->Format.SampleRate = sfinfo.samplerate;

    // set scale factor for file containing floats as recommended by:
    // http://www.mega-nerd.com/libsndfile/api.html#note2
    switch (this->sfinfo.format & SF_FORMAT_SUBMASK)
    {
        case SF_FORMAT_VORBIS:
            [[fallthrough]];
        case SF_FORMAT_FLOAT:
            [[fallthrough]];
        case SF_FORMAT_DOUBLE:
            this->Format.SampleFormat = SampleFormat_t::float32;
            break;
        default:
            this->Format.SampleFormat = SampleFormat_t::int32;
            break;
    }
}

void LibSNDWrapper::close() noexcept
{
    if (this->sndfile != nullptr)
    {
        sf_close(this->sndfile);
        this->sndfile = nullptr;
    }
}

void LibSNDWrapper::fillBuffer()
{
    if (this->data == nullptr)
    {
        if (this->fileOffset.hasValue)
        {
            sf_seek(this->sndfile, msToFrames(this->fileOffset.Value, this->Format.SampleRate), SEEK_SET);
        }
    }

    StandardWrapper::fillBuffer(this);
}

void LibSNDWrapper::render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender)
{
    // what we are doing below is nothing more than efficiently reading audio frames from libsndfile. it got a bit more complex due to
    //   - performance improvements (avoid unnecessary to in conversion if file contains floats)
    //   - supporting the pretty rare ILP64 ABI (unfortuately libsndfile does not provide fixed integer width API, we can only ask to read ints from it which we then have to force to int32)

    constexpr bool haveInt32 = sizeof(int) == 4;
    constexpr bool haveInt64 = sizeof(int) == 8;

    static_assert(haveInt32 || haveInt64, "sizeof(int) is neither 4 nor 8 bits on your platform");

    const SampleFormat_t cachedFormat = this->Format.SampleFormat;

    if (haveInt32)
    {
        // no extra work necessary here, as usual write directly to pcm buffer
        STANDARDWRAPPER_RENDER(
        sndfile_sample_t,
        if (cachedFormat == SampleFormat_t::float32) {
            // the file contains floats, thus read floats
            sf_read_float(this->sndfile, reinterpret_cast<float *>(pcm), framesToDoNow * Channels);
        } else if (cachedFormat == SampleFormat_t::int32) {
            // or read int32s
            sf_read_int(this->sndfile, reinterpret_cast<int32_t *>(pcm), framesToDoNow * Channels);
        } else THROW_RUNTIME_ERROR("THIS SHOULD NEVER HAPPEN: SampleFormat neither int32 nor float32");)
    }
    else if (haveInt64)
    {
        // allocate an extra tempbuffer to store the int64s in
        // then convert them to int32

        vector<int> tmp;
        int *int64_temp_buf = nullptr;
        if (cachedFormat == SampleFormat_t::int32)
        {
            tmp.reserve(gConfig.FramesToRender * this->Format.Channels());
            int64_temp_buf = tmp.data();
        }

        STANDARDWRAPPER_RENDER(
        sndfile_sample_t, // bufferToFill shall be of this type
        if (cachedFormat == SampleFormat_t::float32) {
            // just read out floats
            sf_read_float(this->sndfile, reinterpret_cast<float *>(pcm), framesToDoNow * Channels);
        } else if (cachedFormat == SampleFormat_t::int32) {
            // hm, int is 64 bits wide. write it to temp buffer and force it to int32 and amplify it within the same run.
            sf_read_int(this->sndfile, int64_temp_buf, framesToDoNow * Channels);

            for (unsigned int i = 0; i < framesToDoNow * Channels; i++)
            {
                // see http://www.mega-nerd.com/libsndfile/api.html#note1:
                // Whenever integer data is moved from one sized container to another sized container, the
                // most significant bit in the source container will become the most significant bit in the destination container.
                pcm[i].i = static_cast<int32_t>(int64_temp_buf[i] >> ((sizeof(int) - sizeof(sndfile_sample_t)) * 8) /*==32*/);
            }
        } else THROW_RUNTIME_ERROR("THIS SHOULD NEVER HAPPEN: SampleFormat neither int32 nor float32");)
    }

    if (cachedFormat == SampleFormat_t::float32)
    {
        this->doAudioNormalization(static_cast<float *>(bufferToFill), framesToRender);
    }
    else if (cachedFormat == SampleFormat_t::int32)
    {
        this->doAudioNormalization(static_cast<int32_t *>(bufferToFill), framesToRender);
    }
}

vector<loop_t> LibSNDWrapper::getLoopArray() const noexcept
{
    vector<loop_t> res;

    if (res.empty())
    {
        SF_INSTRUMENT inst;
        int ret = sf_command(this->sndfile, SFC_GET_INSTRUMENT, &inst, sizeof(inst));
        if (ret == SF_TRUE && inst.loop_count > 0)
        {

            for (int i = 0; i < inst.loop_count; i++)
            {
                loop_t l;
                l.start = inst.loops[i].start;
                l.stop = inst.loops[i].end;

                // WARNING: AGAINST RIFF SPEC ahead!!!
                // quoting RIFFNEW.pdf: "dwEnd: Specifies the endpoint of the loop
                // in samples (this sample will also be played)."
                // however (nearly) every piece of software out there ignores that and
                // specifies the sample excluded from the loop
                // THUS: submit to peer pressure
                l.stop -= 1;

                l.count = inst.loops[i].count;
                res.push_back(l);
            }
        }
        else
        {
            if(this->legacyLoop.stop - this->legacyLoop.start > 0)
            {
                res.push_back(this->legacyLoop);
            }
        }
    }
    return res;
}

frame_t LibSNDWrapper::getFrames() const
{
    int framesAvail = this->sfinfo.frames;

    if (this->fileOffset.hasValue)
    {
        framesAvail -= msToFrames(this->fileOffset.Value, this->Format.SampleRate);
    }

    if (framesAvail < 0)
    {
        framesAvail = 0;
    }

    frame_t totalFrames = this->fileLen.hasValue ? msToFrames(this->fileLen.Value, this->Format.SampleRate) : framesAvail;

    if (totalFrames > framesAvail)
    {
        totalFrames = framesAvail;
    }

    return totalFrames;
}

void LibSNDWrapper::buildMetadata() noexcept
{
#define READ_METADATA(name, id)                      \
    if (sf_get_string(this->sndfile, id) != nullptr) \
    (name = string(sf_get_string(this->sndfile, id)))

    READ_METADATA(this->Metadata.Title, SF_STR_TITLE);
    READ_METADATA(this->Metadata.Artist, SF_STR_ARTIST);
    READ_METADATA(this->Metadata.Year, SF_STR_DATE);
    READ_METADATA(this->Metadata.Album, SF_STR_ALBUM);
    READ_METADATA(this->Metadata.Genre, SF_STR_GENRE);
    READ_METADATA(this->Metadata.Track, SF_STR_TRACKNUMBER);
    READ_METADATA(this->Metadata.Comment, SF_STR_COMMENT);
}
