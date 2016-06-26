#include "LibSNDWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>


LibSNDWrapper::LibSNDWrapper(string filename) : StandardWrapper(filename)
{
    this->initAttr();
}

LibSNDWrapper::LibSNDWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : StandardWrapper(filename, offset, len)
{
    this->initAttr();
}

void LibSNDWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::int32;
}

LibSNDWrapper::~LibSNDWrapper ()
{
    this->releaseBuffer();
    this->close();
}


void LibSNDWrapper::open ()
{
    // avoid multiple calls to open()
    if(this->sndfile!=nullptr)
    {
        return;
    }

    memset(&sfinfo, 0, sizeof (sfinfo));
    if (!(this->sndfile = sf_open (this->Filename.c_str(), SFM_READ, &sfinfo)))
    {
        THROW_RUNTIME_ERROR(sf_strerror (NULL) << " (in File \"" << this->Filename << ")\"");
    };

    if (sfinfo.channels < 1 || sfinfo.channels > 6)
    {
        THROW_RUNTIME_ERROR("channels == " << sfinfo.channels);
    };

    this->Format.Channels = sfinfo.channels;
    this->Format.SampleRate = sfinfo.samplerate;

    // set scale factor for file containing floats as recommended by:
    // http://www.mega-nerd.com/libsndfile/api.html#note2
    switch(this->sfinfo.format & SF_FORMAT_SUBMASK)
    {
    case SF_FORMAT_FLOAT:
    /* fall through */
    case SF_FORMAT_DOUBLE:
        sf_command(this->sndfile, SFC_SET_SCALE_FLOAT_INT_READ, nullptr, SF_TRUE);
        break;
    default:
        break;
    }
}

void LibSNDWrapper::close() noexcept
{
    if(this->sndfile!=nullptr)
    {
        sf_close (this->sndfile);
        this->sndfile=nullptr;
    }
}

void LibSNDWrapper::fillBuffer()
{
    if(this->data==nullptr)
    {
        if(this->fileOffset.hasValue)
        {
            sf_seek(this->sndfile, msToFrames(this->fileOffset.Value, this->Format.SampleRate), SEEK_SET);
        }
    }

    StandardWrapper::fillBuffer(this);
}

void LibSNDWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    STANDARDWRAPPER_RENDER(int32_t,
                           // var length array ahead, it a gnu extension
                           int tempBuf[Config::FramesToRender*this->Format.Channels];
                           sf_read_int(this->sndfile, tempBuf, framesToDoNow*this->Format.Channels);

                           constexpr bool haveInt32 = sizeof(int)==4;
                           constexpr bool haveInt64 = sizeof(int)==8;
                           static_assert(haveInt32 || haveInt64, "sizeof(int) is neither 4 nor 8 bits on your platform");
                           if(haveInt32)
{
    memcpy(pcm, tempBuf, framesToDoNow*this->Format.Channels*sizeof(int));
    }
    else if(haveInt64)
{
    for(unsigned int i=0; i<framesToDoNow*this->Format.Channels; i++)
        {
            // see http://www.mega-nerd.com/libsndfile/api.html#note1:
            // Whenever integer data is moved from one sized container to another sized container, the
            // most significant bit in the source container will become the most significant bit in the destination container.
            pcm[i] = static_cast<int32_t>(tempBuf[i] >> 32);
        }
    })
}

vector<loop_t> LibSNDWrapper::getLoopArray () const noexcept
{
    vector<loop_t> res;

    if(res.empty())
    {
        SF_INSTRUMENT inst;
        int ret = sf_command (this->sndfile, SFC_GET_INSTRUMENT, &inst, sizeof (inst)) ;
        if(ret == SF_TRUE && inst.loop_count > 0)
        {

            for (int i=0; i<inst.loop_count; i++)
            {
                loop_t l;
                l.start = inst.loops[i].start;
                l.stop  = inst.loops[i].end;

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
    }
    return res;
}

frame_t LibSNDWrapper::getFrames () const
{
    int framesAvail = this->sfinfo.frames;

    if(this->fileOffset.hasValue)
    {
        framesAvail -= msToFrames(this->fileOffset.Value, this->Format.SampleRate);
    }

    if(framesAvail < 0)
    {
        framesAvail=0;
    }

    frame_t totalFrames = this->fileLen.hasValue ? msToFrames(this->fileLen.Value, this->Format.SampleRate) : framesAvail;

    if(totalFrames > framesAvail)
    {
        totalFrames = framesAvail;
    }

    return totalFrames;
}

void LibSNDWrapper::buildMetadata() noexcept
{
#define READ_METADATA(name, id) if(sf_get_string(this->sndfile, id) != nullptr) name = string(sf_get_string(this->sndfile, id))

    READ_METADATA (this->Metadata.Title, SF_STR_TITLE);
    READ_METADATA (this->Metadata.Artist, SF_STR_ARTIST);
    READ_METADATA (this->Metadata.Year, SF_STR_DATE);
    READ_METADATA (this->Metadata.Album, SF_STR_ALBUM);
    READ_METADATA (this->Metadata.Genre, SF_STR_GENRE);
    READ_METADATA (this->Metadata.Track, SF_STR_TRACKNUMBER);
    READ_METADATA (this->Metadata.Comment, SF_STR_COMMENT);
}
