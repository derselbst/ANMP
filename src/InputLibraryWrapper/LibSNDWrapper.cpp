#include "LibSNDWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>


LibSNDWrapper::LibSNDWrapper(string filename) : StandardWrapper(filename)
{}

LibSNDWrapper::LibSNDWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : StandardWrapper(filename, offset, len)
{}

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
    if ( (this->sndfile = sf_open (this->Filename.c_str(), SFM_READ, &sfinfo))==nullptr )
    {
        THROW_RUNTIME_ERROR(sf_strerror (nullptr) << " (in File \"" << this->Filename << ")\"");
    };

    if (sfinfo.channels < 1)
    {
        THROW_RUNTIME_ERROR("channels == " << sfinfo.channels);
    };

    // dont know how many voices there can be, must be adjusted by user
    this->Format.SetVoices(1);
    this->Format.VoiceChannels[0] = sfinfo.channels;
    this->Format.SampleRate = sfinfo.samplerate;

    // set scale factor for file containing floats as recommended by:
    // http://www.mega-nerd.com/libsndfile/api.html#note2
    switch(this->sfinfo.format & SF_FORMAT_SUBMASK)
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
    // what we are doing below is nothing more than efficiently reading audio frames from libsndfile. it got a bit more complex due to
    //   - performance improvements (avoid unnecessary to in conversion if file contains floats)
    //   - supporting the pretty rare ILP64 ABI (unfortuately libsndfile does not provide fixed integer width API, we can only ask to read ints from it which we then have to force to int32)

    constexpr bool haveInt32 = sizeof(int)==4;
    constexpr bool haveInt64 = sizeof(int)==8;

    static_assert(haveInt32 || haveInt64, "sizeof(int) is neither 4 nor 8 bits on your platform");

#define SND_AMPLIFY(TYPE, XXX) \
    /* audio normalization factor */\
    const float absoluteGain = (numeric_limits<TYPE>::max()) / (numeric_limits<TYPE>::max() * this->gainCorrection);\
    for(unsigned int i=0; gConfig.useAudioNormalization && i<framesToDoNow*Channels; i++)\
    {\
        pcm[i].XXX = static_cast<TYPE>(pcm[i].XXX * absoluteGain);\
    }
    
    const SampleFormat_t cachedFormat = this->Format.SampleFormat;
    
    if(haveInt32)
    {
        // no extra work necessary here, as usual write directly to pcm buffer
        STANDARDWRAPPER_RENDER_WITH_POST_PROC(
                                sndfile_sample_t,
                                if(cachedFormat == SampleFormat_t::float32)
                                {
                                    // the file contains floats, thus read floats
                                    sf_read_float(this->sndfile, reinterpret_cast<float*>(pcm), framesToDoNow*Channels);
                                    
                                    SND_AMPLIFY(float, f);
                                }
                                else if(cachedFormat == SampleFormat_t::int32)
                                {
                                    // or read int32s
                                    sf_read_int(this->sndfile, reinterpret_cast<int32_t*>(pcm), framesToDoNow*Channels);
                                    
                                    SND_AMPLIFY(int32_t, i);
                                }
                                else THROW_RUNTIME_ERROR("THIS SHOULD NEVER HAPPEN: SampleFormat neither int32 nor float32");
                                , ;
        )
    }
    else if(haveInt64)
    {
        // allocate an extra tempbuffer to store the int64s in
        // then convert them to int32

        vector<int> tmp;
        int* int64_temp_buf = nullptr;
        if(cachedFormat == SampleFormat_t::int32)
        {
            tmp.reserve(gConfig.FramesToRender*this->Format.Channels());
            int64_temp_buf = tmp.data();
        }
        
        STANDARDWRAPPER_RENDER_WITH_POST_PROC(
                                sndfile_sample_t, // bufferToFill shall be of this type
                                if(cachedFormat == SampleFormat_t::float32)
                                {
                                    // just read out floats
                                    sf_read_float(this->sndfile, reinterpret_cast<float*>(pcm), framesToDoNow*Channels);
                                    
                                    SND_AMPLIFY(float, f);
                                }
                                else if(cachedFormat == SampleFormat_t::int32)
                                {
                                    // hm, int is 64 bits wide. write it to temp buffer and force it to int32 and amplify it within the same run.
                                    sf_read_int(this->sndfile, int64_temp_buf, framesToDoNow*Channels);
                                    
                                    /* audio normalization factor */
                                    const float absoluteGain = (numeric_limits<int32_t>::max()) / (numeric_limits<int32_t>::max() * this->gainCorrection);
                                    for(unsigned int i=0; i<framesToDoNow*Channels; i++)
                                    {
                                        // see http://www.mega-nerd.com/libsndfile/api.html#note1:
                                        // Whenever integer data is moved from one sized container to another sized container, the
                                        // most significant bit in the source container will become the most significant bit in the destination container.
                                        pcm[i].i = static_cast<int32_t>(int64_temp_buf[i] >> ((sizeof(int) - sizeof(sndfile_sample_t))*8)/*==32*/ );
                                        
                                        if(gConfig.useAudioNormalization)
                                        {
                                            pcm[i].i = static_cast<int32_t>(pcm[i].i * absoluteGain); // so sry for that nesting...
                                        }
                                    }
                                }
                                else THROW_RUNTIME_ERROR("THIS SHOULD NEVER HAPPEN: SampleFormat neither int32 nor float32");
                                , ; // postprocession already done :/
        )
    }
    
#undef SND_AMPLIFY
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
#define READ_METADATA(name, id) if(sf_get_string(this->sndfile, id) != nullptr) (name = string(sf_get_string(this->sndfile, id)))

    READ_METADATA (this->Metadata.Title, SF_STR_TITLE);
    READ_METADATA (this->Metadata.Artist, SF_STR_ARTIST);
    READ_METADATA (this->Metadata.Year, SF_STR_DATE);
    READ_METADATA (this->Metadata.Album, SF_STR_ALBUM);
    READ_METADATA (this->Metadata.Genre, SF_STR_GENRE);
    READ_METADATA (this->Metadata.Track, SF_STR_TRACKNUMBER);
    READ_METADATA (this->Metadata.Comment, SF_STR_COMMENT);
}
