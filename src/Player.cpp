#include "Player.h"

// Constructors/Destructors
//  

Player::Player () {
}

Player::~Player () { }

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
async(launch::async, this->playInternal);
}


/**
 * @return Song
 */
Song Player::getCurrentSong ()
{
}


/**
 * @param  song
 */
void Player::setCurrentSong (Song& song)
{
// ---LOCK---
this->currentSong = &Song;
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
}


/**
 * resets the playhead to beginning of currentSong
 */
void Player::resetPlayhead ()
{
// reset playhead to beginning of song
this->playhead=ItemsToFrames(currentSong->offset);
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
void Player::playLoop (unsigned int startFrame, unsigned int stopFrame)
{
// while there are sub-loops that need to be played
while(loop l* = this->currentSong.loops.getNextLoop(this->playhead) != nullptr)
{

// if the user requested to seek past the current loop, skip it at all
if(this->playhead > l->end)
{
continue;
}

this->playSamples(playhead, l->start);
// at this point: playhead==l.start
        bool forever = l->count==0;
        while(forever || l->count--)
        {
            // if we play this loop multiple time, make sure we start at the beginning again
            this->seekTo(l.start);
            this->playLoop(l);
            // at this point: playhead=l.end
        }
}

// play rest of file
this->playSamples(playhead, stopFrame);
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
int framesToPlay = stopFrame - (this->playhead % this->currentSong->pcm->Frames);
int channels = this->currentSong->Format->Channels;
// ---UNLOCK---

if(framesToPlay<=0)
{
  // well smth. went wrong...
  // TODO: fancy error msg here
  return;
}

this->playSamples(FramesToItems(framesToPlay));
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

// be sure that in every case we start at offset (esp. when we just run over the buffer and thus start again at the beginning)
offset += this->currentSong->pcm->offset;

const int& channels = this->currentSong->Format.Channels;

// ---UNLOCK---

// this is a very simple form of what we do below
// just hand in every frame separately
// Disadvantage: keeps the CPU very busy
//       for(int i=0; i<floatsToPlay; i+=channels)
//         this->audioDriver->write(pcmBuffer+i, 1, channels) ;

// thus do it by handing in small buffers within the buffer ;)
    const int& FRAMES = Config::FramesToRender;
    const int BUFSIZE = FRAMES * channels;

    int fullTransfers = itemsToPlay / BUFSIZE;
    for(int i=0; i<fullTransfers; i++)
    {
        this->currentSong->pcm->fillBuffer();
        this->audioDriver->write (pcmBuffer, FRAMES /* or more correctly BUFSIZE/channels , its the same*/, channels, offset+(i*BUFSIZE));
    }

    int finalTransfer = itemsToPlay % BUFSIZE;
    this->currentSong->pcm->fillBuffer();
    this->audioDriver->write(pcmBuffer, finalTransfer/channels, channels, offset+(fullTransfers*BUFSIZE));
}
}


/**
 * @return bool
 */
bool Player::playInternal ()
{
}

