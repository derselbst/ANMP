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
const frame_t Config::FramesToRender = 2048;


//**********************************
//       HOW-TO-PLAY SECTION       *
//**********************************

// whether the loop information a track may contain, shall be used
bool Config::useLoopInfo = true;

// play each and every loop that many time, -1 will ignore this setting
int Config::overridingGlobalLoopCount = -1;

// time in milliseconds to fade out a song (before it ends), if supported by the underlying WrapperLib
unsigned int defaultFadeTime = 3500;

// time in milliseconds to fade out a song when the user hits stop
unsigned int fadeTimeStop = 3500;

// time in milliseconds to fade out a song when the user hits pause
unsigned int fadeTimePause = 3500;

//**********************************
//      USF-SPECIFIC SECTION       *
//**********************************

// whether to use High-Level-Emulation or not
// true: speed up emulation at the cost of accuracy and potential emulation bugs
bool Config::useHle = false;