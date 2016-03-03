#include <iostream>
#include <limits>

#include "Config.h"
#include "CommonExceptions.h"
#include "Player.h"
#include "ALSAOutput.h"

// TODO: make this nicer
#define FramesToItems(x) ((x)*this->currentSong->Format.Channels)
#define ASYNC_FILL_BUFFER (async(launch::async, &Song::fillBuffer, this->currentSong))
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

    try
    {
        this->futureFillBuffer.wait();
    }
    catch(future_error& e)
    {}

    try
    {
        this->futurePlayInternal.wait();
    }
    catch(future_error& e)
    {}

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

    // unset currentSong, this should force setCurrentSong to reinit audioDriver
    this->currentSong=nullptr;
    this->setCurrentSong(this->playlist->current());
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

    try
    {
        this->futurePlayInternal.wait();
    }
    catch(future_error& e)
    {}

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

    SongFormat oldformat;
    if(this->currentSong != nullptr)
    {
        // wait for another ASYNC_FILL_BUFFER call to finish
        this->futureFillBuffer.wait();
        this->currentSong->releaseBuffer();
        oldformat = this->currentSong->Format;
    }

    this->currentSong = song;
    // go ahead and start filling the pcm buffer
    this->futureFillBuffer = ASYNC_FILL_BUFFER;

    SongFormat& format = this->currentSong->Format;

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
    this->resetPlayhead();
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
      // by draining the last few frames
      this->audioDriver->drain();
    }
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
	if(this->playhead < (*it).start && (*it).start < compareLoop.start)
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
    while((subloop = this->getNextLoop(loop)) != nullptr)
    {

	// if the user requested to seek past the current loop, skip it at all
	if(this->playhead > (*(*subloop)).stop)
	{
	    continue;
	}

	this->playFrames(playhead, (*(*subloop)).start);
	// at this point: playhead==subloop.start
	bool forever = (*(*subloop)).count==0;
	uint32_t count = (*(*subloop)).count;
	while(this->isPlaying && (forever || count--))
	{
	    // if we play this loop multiple time, make sure we start at the beginning again
	    this->seekTo((*(*subloop)).start);
	    this->playLoop(*subloop);
	    // at this point: playhead=l.end
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

// just do nothing with it, but avoid compiler warning
    (void)startFrame;

    do
    {
        if(this->currentSong->getFrames()==0)
	{
	  return;
	}
        signed long long framesToPlay = stopFrame - (this->playhead % this->currentSong->getFrames());

        if(framesToPlay<=0)
        {
            // well smth. went wrong...
            // TODO: fancy error msg here
            return;
        }

        this->playFrames(FramesToItems(framesToPlay));
// here playhead is expected to be equal stopFrame
// if this is not the case play again if necessary
    } while(this->playhead % stopFrame != 0 && this->isPlaying);
}


/**
 * @param  itemsToPlay how many items (e.g. floats, not frames!!) from buffer shall
 * be played
 */
void Player::playFrames (unsigned int itemsToPlay)
{   USERS_ARE_STUPID
    // wait for another ASYNC_FILL_BUFFER call to finish, so that data and bufSize can get updated
    this->futureFillBuffer.wait();

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
        // wait for another fillBuffer call to finish
        this->futureFillBuffer.wait();
        this->audioDriver->write (pcmBuffer, FRAMES /* or more correctly BUFSIZE/channels , its the same*/, offset+(i*BUFSIZE));
        this->futureFillBuffer = ASYNC_FILL_BUFFER;

        // update playhead position
        this->playhead+=FRAMES;
    }

    if(this->isPlaying && (this->playhead==memorizedPlayhead + fullTransfers*FRAMES))
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

