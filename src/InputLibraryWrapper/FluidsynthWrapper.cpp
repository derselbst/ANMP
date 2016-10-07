#include "FluidsynthWrapper.h"

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <cstring>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>


FluidsynthWrapper::FluidsynthWrapper(string filename, string soundfont) : StandardWrapper(filename)
{
    this->initAttr(soundfont);
}

// FluidsynthWrapper::FluidsynthWrapper(string filename, string soundfont, Nullable<size_t> offset, Nullable<size_t> len) : StandardWrapper(filename, offset, len)
// {
//     this->initAttr(soundfont);
// }

void FluidsynthWrapper::initAttr(string soundfont)
{
    this->soundfontFile = soundfont;
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
              THROW_RUNTIME_ERROR("Failed to create the settings");
      }
    fluid_settings_setstr(this->settings, "synth.reverb.active", Config::FluidsynthEnableReverb ? "yes" : "no");
    fluid_settings_setstr(this->settings, "synth.chorus.active", Config::FluidsynthEnableChorus ? "yes" : "no");
    fluid_settings_setnum(this->settings, "synth.sample-rate", Config::FluidsynthSampleRate );

    // these maybe needed for fast renderer (even fluidsynth itself isnt sure about)
    fluid_settings_setstr(this->settings, "player.timing-source", "sample");  
    fluid_settings_setint(this->settings, "synth.parallel-render", 0);
    
      /* Create the synthesizer */
      this->synth = new_fluid_synth(this->settings);
      if (this->synth == nullptr)
      {
              THROW_RUNTIME_ERROR("Failed to create the synth");
      }

      /* Load the soundfont */
      if (!fluid_is_soundfont(this->soundfontFile.c_str()))
      {
              THROW_RUNTIME_ERROR("This is no SF2 (weak)");
      }
      
      if (fluid_synth_sfload(this->synth, this->soundfontFile.c_str(), true) == -1)
      {
              THROW_RUNTIME_ERROR("This is no SF2 (strong)");
      }

      this->player = new_fluid_player(this->synth);
      if (fluid_is_midifile(this->Filename.c_str()))
      {
            fluid_player_add(this->player, this->Filename.c_str());
      }
      else
      {
        THROW_RUNTIME_ERROR("This is no midi file");
      }

    if(!this->fileLen.hasValue)
    {
        this->dryRun();
    }
    
    fluid_player_play(player);
    
    this->Format.Channels = Config::FluidsynthMultiChannel ? 16 : 2;
    this->Format.SampleRate = Config::FluidsynthSampleRate;

}

#include "fluid_player_private.h"

void FluidsynthWrapper::dryRun()
{
    fluid_player_t* localPlayer = new_fluid_player(this->synth);

    fluid_player_add(localPlayer, this->Filename.c_str());

    // setup a custom midi handler, that just returns, so no midievents are being sent to fluid's synth
    // we just need playtime of that midi, no synthesizing, no voices, NOTHING ELSE!
    fluid_player_set_playback_callback(localPlayer,
                                       [](void* data, fluid_midi_event_t* event) -> int { return FLUID_OK; },
                                       this->synth);
    
    /* play the midi files, if any */
    fluid_player_play(localPlayer);
    
    constexpr short chan = 2;
    float left[Config::FramesToRender];
    float right[Config::FramesToRender];
    float* buf[chan]={left, right};
    
    while (fluid_player_get_status(localPlayer) == FLUID_PLAYER_PLAYING)
    {
      if (fluid_synth_process(this->synth, Config::FramesToRender, 0, nullptr, chan, buf) != FLUID_OK)
      {
        break;
      }
    }
    
    this->fileLen = localPlayer->cur_msec - localPlayer->begin_msec;
    
    
    /* wait for playback termination */
    fluid_player_join(localPlayer);
    /* cleanup */
    delete_fluid_player(localPlayer);
}

void FluidsynthWrapper::close() noexcept
{
  if(this->player != nullptr)
  {
    fluid_player_stop(this->player);
    fluid_player_join(this->player);
  }
    delete_fluid_player(this->player);
    this->player = nullptr;
    
    delete_fluid_synth(this->synth);
    this->synth = nullptr;
    
    delete_fluid_settings(this->settings);
    this->settings = nullptr;
}

void FluidsynthWrapper::fillBuffer()
{
    StandardWrapper::fillBuffer(this);
}

void FluidsynthWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    float** temp_buf = new float*[this->Format.Channels];
    
    for(unsigned int i = 0; i< this->Format.Channels; i++)
    {
      temp_buf[i] = new float[Config::FramesToRender];
    }
    
    if(framesToRender==0)
    {
        /* render rest of file */ 
        framesToRender = this->getFrames()-this->framesAlreadyRendered;
    }
    else
    {
        framesToRender = min(framesToRender, this->getFrames()-this->framesAlreadyRendered);
    }
    fesetround(FE_TONEAREST);

    float* pcm = static_cast<float*>(bufferToFill);
    pcm += (this->framesAlreadyRendered * this->Format.Channels) % this->count;

    while(framesToRender>0 && !this->stopFillBuffer)
    {
        int framesToDoNow = (framesToRender/Config::FramesToRender)>0 ? Config::FramesToRender : framesToRender%Config::FramesToRender;

        /* render to raw pcm*/
        fluid_synth_process(this->synth, framesToDoNow, 0, nullptr, this->Format.Channels, temp_buf);                
        for(int frame=0; frame<framesToDoNow; frame++)
          for(unsigned int c=0; c<this->Format.Channels; c++)
          {
              pcm[c * this->Format.Channels + frame] = temp_buf[c][frame];
          };

        /* audio normalization */
        /*const*/ float absoluteGain = (numeric_limits<float>::max()) / (numeric_limits<float>::max() * this->gainCorrection);
        /* reduce risk of clipping, remove that when using true sample peak */
        absoluteGain -= 0.01;
        for(unsigned int i=0; Config::useAudioNormalization && i<framesToDoNow*this->Format.Channels; i++)
        {
            /* simply casting the result of the multiplication could be expensive, since the pipeline of the FPU */
            /* might be flushed. it not very precise either. thus better round here. */
            /* see: http://www.mega-nerd.com/FPcast/ */
            pcm[i] = static_cast<float>(lrint(pcm[i] * absoluteGain));
        }

        pcm += (framesToDoNow * this->Format.Channels) % this->count;
        this->framesAlreadyRendered += framesToDoNow;

        framesToRender -= framesToDoNow;
    }
    
//     STANDARDWRAPPER_RENDER(float,
//                         
//                             fluid_synth_process(this->synth, framesToDoNow, 0, nullptr, this->Format.Channels, temp_buf);
//                             
//                             for(int frame=0; frame<framesToDoNow; frame++)
//                               for(unsigned int c=0; c<this->Format.Channels; c++)
//                             {
//                                 pcm[c * this->Format.Channels + frame] = temp_buf[c][frame];
//                             }
//                           )

    for(unsigned int i=0; i < this->Format.Channels; i++)
    {
            delete[] temp_buf[i];
    }

    delete[] temp_buf;
}

vector<loop_t> FluidsynthWrapper::getLoopArray () const noexcept
{
    vector<loop_t> res;
/*
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
    }*/
    return res;
}

frame_t FluidsynthWrapper::getFrames () const
{
    return msToFrames(this->fileLen.Value, this->Format.SampleRate);
}

void FluidsynthWrapper::buildMetadata() noexcept
{
// #define READ_METADATA(name, id) if(sf_get_string(this->sndfile, id) != nullptr) name = string(sf_get_string(this->sndfile, id))
// 
//     READ_METADATA (this->Metadata.Title, SF_STR_TITLE);
//     READ_METADATA (this->Metadata.Artist, SF_STR_ARTIST);
//     READ_METADATA (this->Metadata.Year, SF_STR_DATE);
//     READ_METADATA (this->Metadata.Album, SF_STR_ALBUM);
//     READ_METADATA (this->Metadata.Genre, SF_STR_GENRE);
//     READ_METADATA (this->Metadata.Track, SF_STR_TRACKNUMBER);
//     READ_METADATA (this->Metadata.Comment, SF_STR_COMMENT);
}
