#include "FluidsynthWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>


FluidsynthWrapper::FluidsynthWrapper(string filename) : StandardWrapper(filename)
{
    this->initAttr();
}

FluidsynthWrapper::FluidsynthWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len) : StandardWrapper(filename, offset, len)
{
    this->initAttr();
}

void FluidsynthWrapper::initAttr()
{
    this->Format.SampleFormat = SampleFormat_t::float32;
}

FluidsynthWrapper::~FluidsynthWrapper ()
{
    this->releaseBuffer();
    this->close();
}


void FluidsynthWrapper::open ()
{
    // avoid multiple calls to open()
    if(this->settings!=nullptr)
    {
        return;
    }

      this->settings = new_fluid_settings();
      if (this->settings == nullptr)
      {
              fprintf(stderr, "Failed to create the settings\n");
      }
    fluid_settings_setstr(settings, "synth.reverb.active", "yes");
    fluid_settings_setstr(settings, "synth.chorus.active", "no");
    fluid_settings_setnum(settings, "synth.sample-rate", 48000 );

      /* Create the synthesizer */
      this->synth = new_fluid_synth(settings);
      if (this->synth == nullptr)
      {
              fprintf(stderr, "Failed to create the synthesizer\n");
      }

      /* Load the soundfont */
      if (!fluid_is_soundfont(this->soundfontFile.c_str()))
      {
        error;
      }
      
      if (fluid_synth_sfload(this->synth, this->soundfontFile.c_str(), true) == -1)
      {
              fprintf(stderr, "Failed to load the SoundFont\n");
      }

      if (fluid_is_midifile(argv[i]))
      {
            fluid_player_add(player, argv[i]);
      }
//       /* Create the audio driver. As soon as the audio driver is
//         * created, the synthesizer can be played. */
//       adriver = new_fluid_audio_driver(settings, synth);
//       if (adriver == NULL) {
//               fprintf(stderr, "Failed to create the audio driver\n");
//               err = 5;
//               goto cleanup;
//       }
//     
    
    
    
    
    this->Format.Channels = sfinfo.channels;
    this->Format.SampleRate = sfinfo.samplerate;

}

void FluidsynthWrapper::close() noexcept
{
    if(this->sndfile!=nullptr)
    {
        sf_close (this->sndfile);
        this->sndfile=nullptr;
    }
}

void FluidsynthWrapper::fillBuffer()
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

void FluidsynthWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
  // unfortuately libsndfile does not provide fixed integer width API
  // we can only ask to read ints from it which we then have to force to int32
  // so here we have to handle the case for architectures where int is 64 bit wide
  
  constexpr bool haveInt32 = sizeof(int)==4;
  constexpr bool haveInt64 = sizeof(int)==8;
  
  static_assert(haveInt32 || haveInt64, "sizeof(int) is neither 4 nor 8 bits on your platform");
  
  if(haveInt32)
  {
    // no extra work necessary here, as usual write directly to pcm buffer
    STANDARDWRAPPER_RENDER(int32_t, sf_read_int(this->sndfile, pcm, framesToDoNow*this->Format.Channels))
  }
  else if(haveInt64)
  {
    // allocate an extra tempbuffer to store the int64s in
    // then convert them to int32
    
    int* int64_temp_buf = new int[Config::FramesToRender*this->Format.Channels];
    
    STANDARDWRAPPER_RENDER(int32_t,
                        
                            sf_read_int(this->sndfile, int64_temp_buf, framesToDoNow*this->Format.Channels);
                            for(unsigned int i=0; i<framesToDoNow*this->Format.Channels; i++)
                            {
                                // see http://www.mega-nerd.com/libsndfile/api.html#note1:
                                // Whenever integer data is moved from one sized container to another sized container, the
                                // most significant bit in the source container will become the most significant bit in the destination container.
                                pcm[i] = static_cast<int32_t>(int64_temp_buf[i] >> 32);
                            }
                          )
    delete [] int64_temp_buf;
  }
}

vector<loop_t> FluidsynthWrapper::getLoopArray () const noexcept
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

frame_t FluidsynthWrapper::getFrames () const
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

void FluidsynthWrapper::buildMetadata() noexcept
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
