
#include "Config.h"
#include "CommonExceptions.h"
#include "Player.h"
#include "ALSAOutput.h"

// TODO: make this nicer
#define FramesToItems(x) ((x)*this->currentSong->pcm->Format.Channels)
#define ASYNC_FILL_BUFFER (async(launch::async, &PCMHolder::fillBuffer, this->currentSong->pcm))

// Constructors/Destructors
//  

Player::Player (IPlaylist* playlist)
{
this->playlist = playlist;
}

Player::~Player ()
{
  if(this->audioDriver!=nullptr)
  {
  this->audioDriver->close();
  }
  delete this->audioDriver;
  
  delete this->playlist;
}

void Player::init()
{
  delete this->audioDriver;
  
  switch(Config::audioDriver)
  {
      case ALSA:
	this->audioDriver = new ALSAOutput();
      break;
    default:
      throw NotImplementedException();
      break;
  }
  
  this->audioDriver->open();
  
  
  this->setCurrentSong(this->playlist->current());
  
}

//  
// Methods
//  


// Accessor methods
//  


// Other methods
//  


/**
 */
void Player::play ()
{
if (this->isPlaying)
{
  return;
}

this->isPlaying=true;

// new thread that runs playInternal
async(launch::async, &Player::playInternal, this);
}


/**
 * @return Song
 */
Song Player::getCurrentSong ()
{
//  return this->currentSong;
  throw NotImplementedException();
}


/**
 * @param  song
 */
void Player::setCurrentSong (Song* song)
{
// ---LOCK---
  if(this->currentSong != nullptr)
  {
    this->currentSong->pcm->releaseBuffer();
  }
this->currentSong = song;
// go ahead and start filling the pcm buffer
this->futureFillBuffer = ASYNC_FILL_BUFFER;
// ---UNLOCK---
this->resetPlayhead();
}


/**
 */
void Player::stop ()
{
this->isPlaying=false;

this->resetPlayhead();
}


/**
 */
void Player::pause ()
{
this->isPlaying=false;

}


/**
 * prepares the player for the next song to be played, but doesnt start playback
 */
void Player::next ()
{
// get next song in playlist

// if channels or samplerate different
//     this->stop();
//     this->setCurrentSong(NextSong);
//     this->audioDriver->init();
// else
//     this->setCurrentSong(NextSong);
}


/**
 */
void Player::previous ()
{
}


/**
 */
void Player::fadeout ()
{
}


/**
 * @param  frame seeks the playhead to frame "frame"
 */
void Player::seekTo (unsigned int frame)
{
  // TODO: stop playback
  this->playhead=frame;
  // TODO: restart playback
}


/**
 * resets the playhead to beginning of currentSong
 */
void Player::resetPlayhead ()
{
  // TODO: stop playback
// reset playhead to beginning of song
this->playhead=0;

// TODO: restart playback??
}


/**
 * make sure you called seekTo(startFrame) before calling this!
 * @param  startFrame intended: the frame with which we want to start the playback
 * 
 * actually: does nothing, just for debug purposes, the actual start is determined
 * by playhead
 * @param  stopFrame play until we've reached stopFrame, although this frame will
 * not be played
 */
void Player::playLoop (core::tree<loop_t>& loop)
{
/* TODO IMPLEMENT ME
// while there are sub-loops that need to be played
while(NODE* l = this->currentSong.loops.getNextLoop(this->playhead) != nullptr)
{

// if the user requested to seek past the current loop, skip it at all
if(this->playhead > l->end)
{
continue;
}

this->playFrames(playhead, l->start);
// at this point: playhead==l.start
        bool forever = l->count==0;
        while(this->isPlaying && (forever || l->count--))
        {
            // if we play this loop multiple time, make sure we start at the beginning again
            this->seekTo(l.start);
            this->playLoop(l);
            // at this point: playhead=l.end
        }
}
*/
// play rest of file
this->playFrames(playhead, (*loop).stop);
}


/**
 * @param  startFrame
 * @param  stopFrame
 */
void Player::playFrames (unsigned int startFrame, unsigned int stopFrame)
{

// just do nothing with it, but avoid compiler warning
(void)startFrame;

// ---LOCK---
int framesToPlay = stopFrame - (this->playhead % this->currentSong->pcm->getFrames());
// ---UNLOCK---

if(framesToPlay<=0)
{
  // well smth. went wrong...
  // TODO: fancy error msg here
  return;
}

this->playFrames(FramesToItems(framesToPlay));
}


/**
 * @param  itemsToPlay how many items (e.g. floats, not frames!!) from buffer shall
 * be played
 */
void Player::playFrames (unsigned int itemsToPlay)
{
// ---LOCK---

size_t bufSize = this->currentSong->pcm->count;

// first define a nice shortcut to our pcmBuffer
pcm_t* pcmBuffer = this->currentSong->pcm->data;

// then seek within the buffer to that point where the playhead points to, but make sure we dont run over the buffer; in doubt we should start again at the beginning of the buffer
int offset = FramesToItems(this->playhead) % bufSize;

// TODO: remove this
// // be sure that in every case we start at offset (esp. when we just run over the buffer and thus start again at the beginning)
// offset += this->currentSong->pcm->offset;

const unsigned int& channels = this->currentSong->pcm->Format.Channels;

// ---UNLOCK---

// this is a very simple form of what we do below
// just hand in every frame separately
// Disadvantage: keeps the CPU very busy
//       for(int i=0; i<floatsToPlay; i+=channels)
//         this->audioDriver->write(pcmBuffer+i, 1, channels) ;

// thus do it by handing in small buffers within the buffer ;)
    const unsigned int& FRAMES = Config::FramesToRender;
    const unsigned int BUFSIZE = FRAMES * channels;

    int fullTransfers = itemsToPlay / BUFSIZE;
    for(int i=0; this->isPlaying && i<fullTransfers; i++)
    {
        // wait for another fillBuffer call to finish
        this->futureFillBuffer.wait();
        this->audioDriver->write (pcmBuffer, FRAMES /* or more correctly BUFSIZE/channels , its the same*/, offset+(i*BUFSIZE));
        this->futureFillBuffer = ASYNC_FILL_BUFFER;
	
	// update playhead position
	this->playhead+=FRAMES;
    }
    
    if(this->isPlaying)
    {
    int finalTransfer = itemsToPlay % BUFSIZE;
    this->futureFillBuffer.wait();
    this->audioDriver->write(pcmBuffer, finalTransfer/channels, offset+(fullTransfers*BUFSIZE));
    this->playhead+=finalTransfer/channels;
    this->futureFillBuffer = ASYNC_FILL_BUFFER;
}
}


void Player::playInternal ()
{
  while(this->isPlaying)
  {
  SongFormat& format = this->currentSong->pcm->Format;
  this->audioDriver->init(format.SampleRate, format.Channels, format.SampleFormat);
  
    core::tree<loop_t>& loops = this->currentSong->loops;
    this->playLoop(loops);
    
    this->next();
  }
}

