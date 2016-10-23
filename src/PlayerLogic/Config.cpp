#include "Config.h"

#include <string>

// Init Static Public attributes
//

//**********************************
//     GLOBAL INTERNAL SECTION     *
//**********************************

// indicates the default audio driver to use
// you have to call Player::initAudio() for changes to take effect
AudioDriver_t Config::audioDriver = AudioDriver_t::ALSA;

// number of frames that are pushed to audioDriver during each run AND minimum no. of frames
// that have to be prepared (i.e. rendered) by a single call to Song::fillBuffer()
constexpr frame_t Config::FramesToRender/* = 2048*/;

// in synchronous part of Song::fillBuffer(): prepare as many samples as needed to have enough for
// PreRenderTime of playing back duration, before asynchronously filling up the rest of PCM buffer
//
// time in milliseconds
//
// high values will slow down the start of playing back the next song, but will assure that the
// asynchronous part of Song::fillBuffer() has enough headstart (i.e. the PCM buffer is well filled)
// so there are no audible glitches later on (such as parts of old songs or cracks and clicks...)
unsigned int Config::PreRenderTime = 500;

// indicates whether the currently playing audiofile shall be only decoded once and held in memory as a whole (true)
// or if only a small buffer shall be allocated holding only Config::FramesToRender frames at one time
// can be set to false, if user needs to save memory, however this will also make seeking within the file impossible
bool Config::RenderWholeSong = true;

// whether to use the audio normalization information generated by anmp-normalize or not
bool Config::useAudioNormalization = true;

//**********************************
//       HOW-TO-PLAY SECTION       *
//**********************************

// whether the loop information a track may contain, shall be used
bool Config::useLoopInfo = true;

// play each and every loop that many time, -1 will ignore this setting
int Config::overridingGlobalLoopCount = -1;

// time in milliseconds to fade out a song when the user hits stop
unsigned int Config::fadeTimeStop = 3500;

// time in milliseconds to fade out a song when the user hits pause
unsigned int Config::fadeTimePause = 300;

//**********************************
//      USF-SPECIFIC SECTION       *
//**********************************

// whether to use High-Level-Emulation (HLE) or not (i.e. Low-Level-Emulation (LLE))
// true: speed up emulation (about factor 2 or more) at the cost of accuracy and potential emulation bugs
// unless you are audiophil, there is no reason to set this to false, since HLE became pretty accurate over the last years
// however you should definitly set this to true when being on a mobile device
bool Config::useHle = false;

//**********************************
//    LIBGME-SPECIFIC SECTION      *
//**********************************

unsigned int Config::gmeSampleRate = 48000;

bool Config::gmeAccurateEmulation = true;

/* Adjust stereo echo depth, where 0.0 = off and 1.0 = maximum. Has no effect for
GYM, SPC, and Sega Genesis VGM music */
float Config::gmeEchoDepth = 0.2f;

bool Config::gmePlayForever = false;

//**********************************
//      MIDI-GENERAL SECTION       *
//**********************************

// no. of the MIDI CC that indicates the start of a looped section
uint8_t Config::MidiControllerLoopStart = 102;

// no. of the MIDI CC that indicates the stop of a looped section
uint8_t Config::MidiControllerLoopStop = 103;

//**********************************
//   FLUIDSYNTH-SPECIFIC SECTION   *
//**********************************

// overrides Config::FramesToRender for fluidsynth
int Config::FluidsynthPeriodSize = 64;

// enables reverb in fluidsynth's synth
bool Config::FluidsynthEnableReverb = true;

// enables chorus in fluidsynth's synth
bool Config::FluidsynthEnableChorus = true;

// write each midi channel to a separate stereo channel (i.e. 32 channel raw pcm channels)
// only use this when writing to file or maybe when using jackd
bool Config::FluidsynthMultiChannel = false;

// the sample rate at with fluidsynth will synthesize
unsigned int Config::FluidsynthSampleRate = 48000;

// always use the default soundfont
bool Config::FluidsynthForceDefaultSoundfont = false;

// the soundfont to use, if there are no more suitable soundfonts found
// this should be a GM Midi conform SF2
string Config::FluidsynthDefaultSoundfont = "/home/tom/Musik/Banjo-Kazooie [Banjo to Kazooie no Daibouken] (1998-05-31)(Rare)(Nintendo)/BK.sf2";

// parameters for fluidsynth's reverb effect
double Config::FluidsynthRoomSize = 0.8;
double Config::FluidsynthDamping = 0.01;
int Config::FluidsynthWidth = 0;
double Config::FluidsynthLevel = 0.8;
