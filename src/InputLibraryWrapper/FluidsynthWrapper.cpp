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

FluidsynthWrapper::FluidsynthWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen) : StandardWrapper(filename, fileOffset, fileLen)
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

void FluidsynthWrapper::setupSeq()
{
    this->sequencer = new_fluid_sequencer2(false /*i.e. use sample timer*/);

    // register synth as first destination
    this->synthSeqId = fluid_sequencer_register_fluidsynth(this->sequencer, this->synth);

    // register myself as second destination
//     mySeqID = fluid_sequencer_register_client(sequencer, "me", seq_callback, NULL);
}

void FluidsynthWrapper::setupSynth()
{
      /* Create the synthesizer */
      this->synth = new_fluid_synth(this->settings);
      if (this->synth == nullptr)
      {
              THROW_RUNTIME_ERROR("Failed to create the synth");
      }
      fluid_synth_set_reverb(this->synth, Config::FluidsynthRoomSize, Config::FluidsynthDamping, Config::FluidsynthWidth, Config::FluidsynthLevel);
      
      // retrieve this after the synth has been inited (just for sure)
      fluid_settings_getint(this->settings, "audio.period-size", &Config::FluidsynthPeriodSize);
      
      
      // find a soundfont
      Nullable<string> soundfont;
      if(!Config::FluidsynthForceDefaultSoundfont)
      {
          soundfont = ::findSoundfont(this->Filename);
      }
      
      if(!soundfont.hasValue)
      {
          // so, either we were forced to use default, or we didnt find any suitable sf2
          soundfont = Config::FluidsynthDefaultSoundfont;
      }
      
      
      /* Load the soundfont */
      if (!fluid_is_soundfont(soundfont.Value.c_str()))
      {
              THROW_RUNTIME_ERROR("This is no SF2 (weak)");
      }
      
      if (fluid_synth_sfload(this->synth, soundfont.Value.c_str(), true) == -1)
      {
              THROW_RUNTIME_ERROR("This is no SF2 (strong)");
      }
}

void FluidsynthWrapper::setupSettings()
{
      this->settings = new_fluid_settings();
      if (this->settings == nullptr)
      {
              THROW_RUNTIME_ERROR("Failed to create the settings");
      }
      
    fluid_settings_setstr(this->settings, "synth.reverb.active", Config::FluidsynthEnableReverb ? "yes" : "no");
    fluid_settings_setstr(this->settings, "synth.chorus.active", Config::FluidsynthEnableChorus ? "yes" : "no");
    
    fluid_settings_setint(this->settings, "synth.min-note-length", 1);
    // only in mma mode, bank high and bank low controllers are handled as specified by MIDI standard
    fluid_settings_setstr(this->settings, "synth.midi-bank-select", "mma");
    
    {
      fluid_settings_setnum(this->settings, "synth.sample-rate", Config::FluidsynthSampleRate);
      double srate;
      fluid_settings_getnum(this->settings, "synth.sample-rate", &srate);
      this->Format.SampleRate = static_cast<unsigned int>(srate);
      
      int stereoChannels = Config::FluidsynthMultiChannel ? 16 : 1;
      fluid_settings_setint(this->settings, "synth.audio-groups",    stereoChannels);
      fluid_settings_setint(this->settings, "synth.audio-channels",  stereoChannels);
      fluid_settings_getint(this->settings, "synth.audio-channels", &stereoChannels);
      this->Format.Channels = stereoChannels * 2;
    }
    
    // these maybe needed for fast renderer (even fluidsynth itself isnt sure about)
    fluid_settings_setstr(this->settings, "player.timing-source", "sample");
    fluid_settings_setint(this->settings, "synth.parallel-render", 0);
    fluid_settings_setint(this->settings, "synth.threadsafe-api", 0);
}

void FluidsynthWrapper::open ()
{
    // avoid multiple calls to open()
    if(this->settings!=nullptr)
    {
        return;
    }
    this->setupSettings();
    
    
        this->smf = smf_load(this->Filename.c_str());
        if (this->smf == NULL)
        {
              THROW_RUNTIME_ERROR("Something is wrong with that midi, loading failed");
        }
        
        double playtime = smf_get_length_seconds(this->smf);
        if(playtime<0.0)
        {
            THROW_RUNTIME_ERROR("How can playtime be negative?!?");
        }
        
        this->fileLen = static_cast<size_t>(playtime * 1000);
    
        this->setupSynth();
        this->setupSeq();
    
    smf_event_t *event;
    while((event = smf_get_next_event(smf)) != nullptr)
    {
        struct
        {
        uint8_t status:4;
        uint8_t chan:4;
        uint8_t param1 = 0;
        uint8_t param2 = 0;
        } mEvt;
        
                if (smf_event_is_metadata(event) || smf_event_is_sysex(event))
                {
                        continue;
                }

                if (!smf_event_length_is_valid(event))
                {
                 // TODO LOG, ignore that event 
                    //g_critical("smf_event_decode: incorrect MIDI message length.");
                 continue;
                }
 
                this->feedToFluidSeq(event);
                
                memcpy(mEvt, event->midi_buffer, min(sizeof(mEvt), event->midi_buffer_length));
                
                switch (mEvt.status) {
                 case 0x8:
                         off += snprintf(buf + off, BUFFER_SIZE - off, "Note Off, channel %d, note %s, velocity %d",
                                         channel, note, event->midi_buffer[2]);
                         break;
 
                 case 0x9:
                         note_from_int(note, event->midi_buffer[1]);
                         off += snprintf(buf + off, BUFFER_SIZE - off, "Note On, channel %d, note %s, velocity %d",
                                         channel, note, event->midi_buffer[2]);
                         break;
 
                 case 0xA:
                         note_from_int(note, event->midi_buffer[1]);
                         off += snprintf(buf + off, BUFFER_SIZE - off, "Aftertouch, channel %d, note %s, pressure %d",
                                         channel, note, event->midi_buffer[2]);
                         break;
 
                 case 0xB:
                         off += snprintf(buf + off, BUFFER_SIZE - off, "Controller, channel %d, controller %d, value %d",
                                         channel, event->midi_buffer[1], event->midi_buffer[2]);
                         break;
 
                case 0xC:
                         off += snprintf(buf + off, BUFFER_SIZE - off, "Program Change, channel %d, controller %d",
                                         channel, event->midi_buffer[1]);
                         break;
 
                 case 0xD:
                         off += snprintf(buf + off, BUFFER_SIZE - off, "Channel Pressure, channel %d, pressure %d",
                                         channel, event->midi_buffer[1]);
                         break;
 
                 case 0xE:
                         off += snprintf(buf + off, BUFFER_SIZE - off, "Pitch Wheel, channel %d, value %d",
                                         channel, ((int)event->midi_buffer[2] << 7) | (int)event->midi_buffer[2]);
                         break;
 
                 default:
                         free(buf);
                         return (NULL);
                
        }
}


// #include "fluid_player_private.h"

// void FluidsynthWrapper::dryRun()
// {
//     fluid_player_t* localPlayer = new_fluid_player(this->synth);
// 
//     fluid_player_add(localPlayer, this->Filename.c_str());
// 
//     // setup a custom midi handler, that just returns, so no midievents are being sent to fluid's synth
//     // we just need playtime of that midi, no synthesizing, no voices, NOTHING ELSE!
//     fluid_player_set_playback_callback(localPlayer,
//                                        [](void* data, fluid_midi_event_t* event) -> int { (void)data;(void)event;return FLUID_OK; },
//                                        this->synth);
//     
//     /* play the midi files, if any */
//     fluid_player_play(localPlayer);
//     
//     constexpr short chan = 2;
//     float left[Config::FramesToRender];
//     float right[Config::FramesToRender];
//     float* buf[chan]={left, right};
//     
//     while (fluid_player_get_status(localPlayer) == FLUID_PLAYER_PLAYING)
//     {
//       if (fluid_synth_process(this->synth, Config::FramesToRender, 0, nullptr, chan, buf) != FLUID_OK)
//       {
//         break;
//       }
//     }
//     
//     this->fileLen = localPlayer->cur_msec - localPlayer->begin_msec;
//     
//     
//     /* wait for playback termination */
//     fluid_player_join(localPlayer);
//     /* cleanup */
//     delete_fluid_player(localPlayer);
// }

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

frame_t FluidsynthWrapper::getFrames () const
{
    return msToFrames(this->fileLen.Value, this->Format.SampleRate);
}

// HACK there seems to be some strange bug in fluidsynth:
// whenever we ask the synth to render something else than exactly 64 frames, we get strange timed audio
// thus obtain the period size (?always? == 64) within this->open() and use this value instead of our Config::FramesToRender
// by replacing Config::FramesToRender with Config::FluidsynthPeriodSize
#define FramesToRender FluidsynthPeriodSize
void FluidsynthWrapper::render(pcm_t* bufferToFill, frame_t framesToRender)
{
    float** temp_buf = new float*[this->Format.Channels];
    
    for(unsigned int i = 0; i< this->Format.Channels; i++)
    {
      temp_buf[i] = new float[Config::FramesToRender];
    }

    STANDARDWRAPPER_RENDER(float,

                            fluid_synth_process(this->synth, framesToDoNow, 0, nullptr, this->Format.Channels, temp_buf);                
                            for(int frame=0; frame<framesToDoNow; frame++)
                              for(unsigned int c=0; c<this->Format.Channels; c++)
                              {
                                  pcm[frame * this->Format.Channels + c] = temp_buf[c][frame];
                              };
                           )

    for(unsigned int i=0; i < this->Format.Channels; i++)
    {
            delete[] temp_buf[i];
    }

    delete[] temp_buf;
}
#undef FramesToRender
