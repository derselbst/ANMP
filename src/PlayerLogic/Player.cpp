#include <iostream>
#include <limits>

#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "Player.h"
#include "ALSAOutput.h"

// TODO: make this nicer
#define FramesToItems(x) ((x)*this->currentSong->Format.Channels)
#define USERS_ARE_STUPID if(this->audioDriver==nullptr || this->playlist==nullptr || this->currentSong==nullptr){throw NotInitializedException();}
// Constructors/Destructors
//

Player::Player (IPlaylist* playlist)
{
    this->playlist = playlist;
}

Player::~Player ()
{
    this->stop();

WAIT(this->futurePlayInternal);

    lock_guard<recursive_mutex> lck(mtxCurrentSong);

    if(this->audioDriver!=nullptr)
    {
        this->audioDriver->close();
    }
    delete this->audioDriver;
}

void Player::init()
{
    this->pause();
    
    lock_guard<recursive_mutex> lck(mtxCurrentSong);

    delete this->audioDriver;

    switch(Config::audioDriver)
    {
    case ALSA:
        this->audioDriver = new ALSAOutput();
        break;
    default:
        this->audioDriver=nullptr;
        throw NotImplementedException();
        break;
    }

    this->audioDriver->open();

      SongFormat& fmt = this->currentSong->Format;
      this->audioDriver->init(fmt.SampleRate, fmt.Channels, fmt.SampleFormat);
      this->resetPlayhead();
}


/**
 */
void Player::play ()
{
  if(this->audioDriver==nullptr || this->currentSong==nullptr)
  {
    this->init();
  }
  
    if (this->isPlaying)
    {
        return;
    }

WAIT(this->futurePlayInternal);

    this->isPlaying=true;

// new thread that runs playInternal
    this->futurePlayInternal = async(launch::async, &Player::playInternal, this);
}


/**
 * @return Song
 */
Song* Player::getCurrentSong ()
{   USERS_ARE_STUPID
//  return this->currentSong;
    throw NotImplementedException();
}


/**
 * @param  song
 */
void Player::setCurrentSong (Song* song)
{
    lock_guard<recursive_mutex> lck(mtxCurrentSong);
    
    this->resetPlayhead();
    if(this->currentSong == song)
    {
      // nothing to do here
      return;
    }

    // capture format of former current song
    SongFormat oldformat;
    if(this->currentSong != nullptr)
    {
        this->currentSong->releaseBuffer();
	this->currentSong->close();
        oldformat = this->currentSong->Format;
    }

    this->currentSong = song;
    if(this->currentSong == nullptr)
    {
      return;
    }
    
    this->currentSong->open();
    // go ahead and start filling the pcm buffer
    this->currentSong->fillBuffer();

    // if the audio has already been initialized
    if(this->audioDriver!=nullptr)
    {
      SongFormat& format = this->currentSong->Format;

      // in case samplerate or channel count differ, reinit audioDriver
      if(oldformat != format)
      {
	  try
	  {
	      this->audioDriver->init(format.SampleRate, format.Channels, format.SampleFormat);
	  }
	  catch(exception& e)
	  {
	      cout << e.what() << endl;
	      this->stop();
	      return;

	  }
      }
    }
}


/** @brief stops the playback
 * same as pause() but also resets playhead
 *
 */
void Player::stop ()
{
    this->pause();
    this->resetPlayhead();
}


/** @brief pauses the playback
 * stops the playback at the next opportunity
 *
 */
void Player::pause ()
{
    this->isPlaying=false;

    if(this->audioDriver!=nullptr)
    {
      // we wont feed any audioDriver with PCM anymore, so stop the PCM stream
      // by dropping the last few frames
      this->audioDriver->drop();
    }
    WAIT(this->futurePlayInternal);
}


/**
 * prepares the player for the next song to be played, but doesnt start playback
 */
void Player::next ()
{   USERS_ARE_STUPID
    this->setCurrentSong(this->playlist->next());
}


/**
 */
void Player::previous ()
{   USERS_ARE_STUPID
    this->setCurrentSong(this->playlist->previous());
}


/**
 */
void Player::fadeout ()
{   USERS_ARE_STUPID
}


/**
 * @param  frame seeks the playhead to frame "frame"
 */
void Player::seekTo (frame_t frame)
{
    this->playhead=frame;
    return;
}


/**
 * resets the playhead to beginning of currentSong
 */
void Player::resetPlayhead ()
{
    this->seekTo(0);
}

core::tree<loop_t>* Player::getNextLoop(core::tree<loop_t>& l)
{
    loop_t compareLoop;
    compareLoop.start = numeric_limits<frame_t>::max();
    
    core::tree<loop_t>* ptrToNearest = nullptr;
    // find loop that starts closest, i.e. right after, to whereever playhead points to
    for (core::tree<loop_t>::iterator it = l.begin(); it != l.end(); ++it)
    {
	if(this->playhead <= (*it).start && (*it).start < compareLoop.start)
	{
	  compareLoop=*it;
	  ptrToNearest=it.tree_ptr();
	}
    }
    
    return ptrToNearest;
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
{   USERS_ARE_STUPID
  
    // while there are sub-loops that need to be played
    core::tree<loop_t>* subloop;
    while(Config::useLoopInfo && ((subloop = this->getNextLoop(loop)) != nullptr))
    {

	// if the user requested to seek past the current loop, skip it at all
	if(this->playhead > (*(*subloop)).stop)
	{
	    continue;
	}

	this->playFrames(playhead, (*(*subloop)).start);
	// at this point: playhead==subloop.start
	
	uint32_t mycount = Config::overridingGlobalLoopCount!=-1 ? Config::overridingGlobalLoopCount : (*(*subloop)).count;
	bool forever = mycount==0;
	while(this->isPlaying && (forever || mycount--))
	{
	    // if we play this loop multiple time, make sure we start at the beginning again
	    this->seekTo((*(*subloop)).start);
	    this->playLoop(*subloop);
	    // at this point: playhead=subloop.end
	}
    }

    // play rest of file
    this->playFrames(playhead, (*loop).stop);
}


/**
 * @param  startFrame
 * @param  stopFrame
 */
void Player::playFrames (frame_t startFrame, frame_t stopFrame)
{   USERS_ARE_STUPID

  if(startFrame!=this->playhead)
  {
    cout << "Oops: Expected Playhead to be equal " << startFrame << ", but playhead is " << this->playhead << endl;
  }

    // the user may request to seek while we are playing, thus check whether playhead is
    // still in range
    while(this->isPlaying && this->playhead>=startFrame && this->playhead<stopFrame)
    {
        if(this->currentSong->getFrames()==0)
	{
	  return;
	}
        signed long long framesToPlay = stopFrame - this->playhead;

        if(framesToPlay<=0)
        {
            // well smth. went wrong...
            cerr << "THIS SHOULD NEVER HAPPEN! framesToPlay negative" << endl;
            return;
        }

        this->playFrames(FramesToItems(framesToPlay));
	// here playhead is expected to be equal stopFrame
	// if this is not the case play again if necessary
    }
}


/**
 * @param  itemsToPlay how many items (e.g. floats, not frames!!) from buffer shall
 * be played
 */
void Player::playFrames (unsigned int itemsToPlay)
{   USERS_ARE_STUPID

// first define a nice shortcut to our pcmBuffer
    pcm_t* pcmBuffer = this->currentSong->data;

    frame_t memorizedPlayhead = this->playhead;

    size_t bufSize = this->currentSong->count;
    if(bufSize==0)
    {
      // nothing to play, we are finished here
      return;
    }
// then seek within the buffer to that point where the playhead points to, but make sure we dont run over the buffer; in doubt we should start again at the beginning of the buffer
    int offset = FramesToItems(memorizedPlayhead) % bufSize;

    const unsigned int& channels = this->currentSong->Format.Channels;

// this is a very simple form of what we do below
// just hand in every frame separately
// Disadvantage: keeps the CPU very busy
//       for(int i=0; i<floatsToPlay; i+=channels)
//         this->audioDriver->write(pcmBuffer+i, 1, channels) ;

// thus do it by handing in small buffers within the buffer ;)
    const frame_t& FRAMES = Config::FramesToRender;
    const unsigned int BUFSIZE = FRAMES * channels;

    int fullTransfers = itemsToPlay / BUFSIZE;
    for(int i=0; this->isPlaying && (this->playhead==memorizedPlayhead + i*FRAMES) && i<fullTransfers; i++)
    {
        this->audioDriver->write (pcmBuffer, FRAMES /* or more correctly BUFSIZE/channels , its the same*/, offset+(i*BUFSIZE));
        this->currentSong->fillBuffer();

        // update playhead position
        this->playhead+=FRAMES;
    }

    if(this->isPlaying && (this->playhead==memorizedPlayhead + fullTransfers*FRAMES))
    {
        int finalTransfer = itemsToPlay % BUFSIZE;
        this->audioDriver->write(pcmBuffer, finalTransfer/channels, offset+(fullTransfers*BUFSIZE));
        this->playhead+=finalTransfer/channels;
        this->currentSong->fillBuffer();
    }
}


void Player::playInternal ()
{
    lock_guard<recursive_mutex> lck(mtxCurrentSong);
    
    this->audioDriver->start();
    while(this->isPlaying)
    {
        core::tree<loop_t>& loops = this->currentSong->loopTree;

        this->playLoop(loops);

        // ok, for some reason we left playLoop, if this was due to we shall stop playing
        // leave this loop immediately, else play next song
        if(!this->isPlaying)
        {
            break;
        }
        this->next();
    }
}

