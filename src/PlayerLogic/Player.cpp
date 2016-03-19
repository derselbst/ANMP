#include "Player.h"

#include "tree.h"
#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"

#include "Playlist.h"
#include "Song.h"
#include "IAudioOutput.h"
#include "ALSAOutput.h"

#include <iostream>
#include <limits>


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

    if(this->audioDriver!=nullptr)
    {
        this->audioDriver->close();
    }
    delete this->audioDriver;
}

void Player::init()
{
    this->pause();

    WAIT(this->futurePlayInternal);

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
    if(this->currentSong==nullptr)
    {
        return;
    }

    if(this->audioDriver==nullptr)
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
    if(this->isPlaying)
    {
        throw runtime_error("Player: Cannot set song while still playing back.");
    }

    WAIT(this->futurePlayInternal);
    this->_setCurrentSong(song);
}

void Player::_setCurrentSong (Song* song)
{
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
        // usually by dropping the last few frames
        this->audioDriver->stop();
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
  if(this->currentSong!=nullptr && 
     this->currentSong->count==FramesToItems(this->currentSong->getFrames()))
  {
    this->_seekTo(frame);
}
}


/**
 * @param  frame seeks the playhead to frame "frame"
 */
void Player::_seekTo (frame_t frame)
{
    this->playhead=frame;  
}


/**
 * resets the playhead to beginning of currentSong
 */
void Player::resetPlayhead ()
{
    this->_seekTo(0);
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
    while(this->isPlaying &&
            Config::useLoopInfo &&
            ((subloop = this->getNextLoop(loop)) != nullptr))
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
                this->_seekTo((*(*subloop)).start);
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

        this->playFrames(framesToPlay);
        // here playhead is expected to be equal stopFrame
        // if this is not the case play again if necessary
    }
}


void Player::playFrames (frame_t framesToPlay)
{   USERS_ARE_STUPID

    frame_t memorizedPlayhead = this->playhead;
    size_t& bufSize = this->currentSong->count;

// here is a very simple form of what we do below
// just hand in every frame separately
//       for(int i=0; i<framesToPlay; i+=channels)
//         this->audioDriver->write(pcmBuffer+i.toItems(), 1, channels) ;
// Disadvantage: keeps the CPU very busy
// thus do it by handing in small buffers within the buffer ;)

    // check if we seeked during playback or pcm size is zero
    while(this->isPlaying && // has smb. stopped playback?
            framesToPlay>0 && // are there any frames left to play?
            bufSize!=0 && // are there any samples in pcm buffer?
            memorizedPlayhead==this->playhead // make sure noone seeked while playing
         )
    {
        // seek within the buffer to that item where the playhead points to, but make sure we dont run over the buffer; in doubt we should start again at the beginning of the buffer
        int itemOffset = FramesToItems(memorizedPlayhead) % bufSize;
        // number of frames we will write to audioDriver in this run
        int framesToPush = min(Config::FramesToRender, framesToPlay);
        // PLAY!
        int framesWritten = this->audioDriver->write(this->currentSong->data, framesToPush, itemOffset);

        // ensure PCM buffer(s) are well filled
        this->currentSong->fillBuffer();

        // update the playhead
        this->playhead+=framesWritten;
	if(this->playheadChanged!=nullptr)
	{
	  this->playheadChanged(this->callbackContext, this->playhead);
	}
	
        // update our local copy of playhead
        memorizedPlayhead+=framesWritten;
        // update frames-left-to-play
        framesToPlay-=framesWritten;
    }
}


void Player::playInternal ()
{
    try
    {
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
            this->_setCurrentSong(this->playlist->next());
        }
    }
    catch(exception& e)
    {
        cerr << "An Exception was thrown in Player::playInternal(): " << e.what() << endl;

        // will be save in futurePlayInternal, not sure if anybody will ever notice though
        throw;
    }
}

