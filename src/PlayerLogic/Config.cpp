#include "Config.h"

// Init Static Public attributes
//

//**********************************
//     GLOBAL INTERNAL SECTION     *
//**********************************

// indicates the default audio driver to use
AudioDriver_t Config::audioDriver = AudioDriver_t::ALSA;

// number of frames that are pushed to audioDriver during each run AND minimum no. of frames
// that have to be prepared (i.e. rendered) by a single call to Song::fillBuffer()
//
// as of 2016-10-09 this is not const anymore, due to my lazyness to implement additional logic for FluidsynthWrapper (which has an ugly API)
// nevertheless this var is still meant to be const! DONT CHANGE IT!
/*const*/ frame_t Config::FramesToRender = 2048;

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

bool Config::useAudioNormalization = true;

//**********************************
//       HOW-TO-PLAY SECTION       *
//**********************************

// whether the loop information a track may contain, shall be used
bool Config::useLoopInfo = true;

// play each and every loop that many time, -1 will ignore this setting
int Config::overridingGlobalLoopCount = -1;

// time in milliseconds to fade out a song (before it ends), if supported by the underlying WrapperLib
unsigned int Config::defaultFadeTime = 3500;

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
//   FLUIDSYNTH-SPECIFIC SECTION   *
//**********************************

bool Config::FluidsynthEnableReverb = true;
bool Config::FluidsynthEnableChorus = true;
bool Config::FluidsynthMultiChannel = false;
unsigned int Config::FluidsynthSampleRate = 48000;
string Config::FluidsynthDefaultSoundfont = "/home/tom/Musik/Donkey Kong 64/DK64.sf2";