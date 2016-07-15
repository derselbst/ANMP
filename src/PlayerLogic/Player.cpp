#include "Player.h"

#include "tree.h"
#include "Config.h"
#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"

#include "Playlist.h"
#include "Song.h"
#include "IAudioOutput.h"

#ifdef USE_ALSA
#include "ALSAOutput.h"
#endif

#ifdef USE_EBUR128
#include "ebur128Output.h"
#endif

#ifdef USE_JACK
#include "JackOutput.h"
#endif

#include "WaveOutput.h"

#ifdef USE_PORTAUDIO
#include "PortAudioOutput.h"
#endif

#include <iostream>
#include <limits>
#include <cmath>

// TODO: make this nicer
#define FramesToItems(x) ((x)*this->currentSong->Format.Channels)
#define SEEK_POSSIBLE (this->currentSong==nullptr ? false : this->currentSong->count==FramesToItems(this->currentSong->getFrames()))
#define USERS_ARE_STUPID if(this->audioDriver==nullptr || this->playlist==nullptr || this->currentSong==nullptr){throw NotInitializedException();}
// Constructors/Destructors
//

Player::Player (IPlaylist* playlist)
{
    this->playlist = playlist;
}

Player::~Player ()
{
    this->pause();

    if(this->audioDriver!=nullptr)
    {
        this->audioDriver->close();
    }
    delete this->audioDriver;
}

void Player::_initAudio()
{
    this->_pause();

    delete this->audioDriver;

    switch(Config::audioDriver)
    {
#ifdef USE_ALSA
    case ALSA:
        this->audioDriver = new ALSAOutput();
        break;
#endif
#ifdef USE_EBUR128
    case ebur128:
        this->audioDriver = new ebur128Output(this);
        break;
#endif
#ifdef USE_JACK
    case JACK:
        this->audioDriver = new JackOutput();
        break;
#endif
    case WAVE:
        this->audioDriver = new WaveOutput(this);
        break;
#ifdef USE_PORTAUDIO
    case PORTAUDIO:
        this->audioDriver = new PortAudioOutput();
        break;
#endif
    default:
        this->audioDriver=nullptr;
        throw NotImplementedException();
        break;
    }

    this->audioDriver->open();
}

bool Player::getIsPlaying()
{
    return this->isPlaying;
}

/**
 */
void Player::play ()
{
    if (this->isPlaying)
    {
        return;
    }

    WAIT(this->futurePlayInternal);

    if(this->currentSong==nullptr)
    {
        Song* s=this->playlist->current();
        this->setCurrentSong(s);
        if(s==nullptr)
        {
            return;
        }
    }

    this->audioDriver->setVolume(this->PreAmpVolume);

    this->isPlaying=true;

// new thread that runs playInternal
    this->futurePlayInternal = async(launch::async, &Player::playInternal, this);
}


/**
 * @return Song
 */
const Song* Player::getCurrentSong ()
{
    return this->currentSong;
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
    // make sure audio driver is initialized
    if(this->audioDriver==nullptr)
    {
        // if successfull this->audioDriver will be != nullptr and opened
        // if not: exception will be thrown
        this->_initAudio();
    }

    if(song == nullptr)
    {
        this->_pause();
        this->currentSong = song;
    }
    else if(song != this->currentSong)
    {
        // capture format of former current song
        SongFormat oldformat;
        if(this->currentSong != nullptr)
        {
            oldformat = this->currentSong->Format;
            this->currentSong->releaseBuffer();
            this->currentSong->close();
        }

        this->currentSong = song;
        // open the audio file
        this->currentSong->open();
        // go ahead and start filling the pcm buffer
        this->currentSong->fillBuffer();

        SongFormat& format = this->currentSong->Format;

        // in case samplerate or channel count differ, reinit audioDriver
        if(oldformat != format)
        {
            this->audioDriver->init(format.SampleRate, format.Channels, format.SampleFormat);
        }
    }

    this->resetPlayhead();

    // now we are ready to do the callback
    this->onCurrentSongChanged.Fire();
}


/*
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
        this->_pause();
        this->onCurrentSongChanged.Fire();
        return;
    }

    this->currentSong->open();
    // go ahead and start filling the pcm buffer
    this->currentSong->fillBuffer();

    if(this->audioDriver==nullptr)
    {
        this->_initAudio();
    }

    // if the audio has bee properly been initialized
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
                this->_pause();
                return;
            }
        }
    }
    else
    {
        cerr << "THIS SHOULD NEVER HAPPEN! audioDriver==nullptr" << endl;
        return;
    }

    this->onCurrentSongChanged.Fire();
}*/


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
    this->_pause();
    WAIT(this->futurePlayInternal);
}

/** @brief pauses the playback
 * stops the playback at the next opportunity
 *
 */
void Player::_pause ()
{
    this->isPlaying=false;

    if(this->audioDriver!=nullptr)
    {
        // we wont feed any audioDriver with PCM anymore, so stop the PCM stream
        // usually by dropping the last few frames
        this->audioDriver->stop();
    }
}

/**
 * prepares the player for the next song to be played, but doesnt start playback
 */
void Player::next ()
{
    this->setCurrentSong(this->playlist->next());
}


/**
 */
void Player::previous ()
{
    this->setCurrentSong(this->playlist->previous());
}


/**
 */
void Player::fadeout (unsigned int fadeTime)
{
    USERS_ARE_STUPID

    if(fadeTime == 0)
    {
        this->audioDriver->setVolume(0);
    }

    float vol = 0.0f;
    for(unsigned int timePast=0; timePast <= fadeTime; timePast++)
    {
        switch (3)
        {
        case 1:
            // linear
            vol =  1.0f - ((float)timePast / (float)fadeTime);
            break;
        case 2:
            // logarithmic
            vol = 1.0f - pow(0.1, (1 - ((float)timePast / (float)fadeTime)) * 1);
            break;
        case 3:
            // sine
            vol =  1.0f - sin( ((float)timePast / (float)fadeTime) * M_PI / 2 );
            break;
        }

        float volToPush = vol*this->PreAmpVolume;
        this->audioDriver->setVolume(volToPush);
        this_thread::sleep_for (chrono::milliseconds(1));
    }
}


/**
 * @param  frame seeks the playhead to frame "frame"
 */
void Player::seekTo (frame_t frame)
{
    if(this->currentSong!=nullptr &&
            SEEK_POSSIBLE)
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



void Player::playLoop (core::tree<loop_t>& loop)
{
    USERS_ARE_STUPID

    // sub-loops that need to be played
    core::tree<loop_t>* subloop;

    while(this->isPlaying && // are we still playing?
            Config::useLoopInfo && // user wishes to use available loop info
            SEEK_POSSIBLE && // we loop by setting the playhead, if this is not possible, since we dont hold the whole pcm, no loops are available
            ((subloop = this->getNextLoop(loop)) != nullptr)) // are there subloops left that need to be played?
    {

        // if the user requested to seek past the current loop, skip it at all
        if(this->playhead > (*(*subloop)).stop)
        {
            cerr << "this code seems to be redundant... NO IT ISNT!!!" << endl;
            continue;
        }

        this->playFrames(playhead, (*(*subloop)).start);
        // at this point: playhead==subloop.start

        uint32_t mycount = Config::overridingGlobalLoopCount!=-1 ? Config::overridingGlobalLoopCount : (*(*subloop)).count;
        bool forever = mycount==0;
        mycount += 1; // +1 because the subloop we are just going to play, should be played one additional time by the parent of subloop (i.e. the loop we are currently in)
        while(this->isPlaying && (forever || mycount--))
        {
            // if we play this loop multiple time, make sure we start at the beginning again
            this->_seekTo((*(*subloop)).start);
            this->playLoop(*subloop);
            // at this point: playhead=subloop.end

            // actually we could leave this loop with playhead==subloop.start, which would avoid subloop.count+1 up ^ there ^, however
            // this would cause an infinite loop since getNextLoop() would return the same subloop again and again
        }
    }

    // play rest of file
    this->playFrames(playhead, (*loop).stop);
}


/**
 * make sure you called seekTo(startFrame) before calling this!
 * @param  startFrame intended: the frame with which we want to start the playback
 * actually: does nothing, just for debug purposes, the actual start is determined
 * by playhead
 * @param  stopFrame play until we've reached stopFrame, although this frame will
 * not be played
 */
void Player::playFrames (frame_t startFrame, frame_t stopFrame)
{
    USERS_ARE_STUPID

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
{
    USERS_ARE_STUPID

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
        // seek within the pcm buffer to that item where the playhead points to, but make sure we dont run over the buffer; in doubt we should start again at the beginning of the buffer
        int itemOffset = FramesToItems(memorizedPlayhead) % bufSize;
        // number of frames we will write to audioDriver in this run
        int framesToPush = min(Config::FramesToRender, framesToPlay);
        
        int framesWritten = 0;

        // PLAY!
        again:
        framesWritten = this->audioDriver->write(this->currentSong->data, framesToPush, itemOffset);
        // before we go on rendering the next pcm chunk, make sure we really played the current one.
        //
        // audioDriver.write() may return a value !=framesToPush.
        // In this case we should call audioDriver.write() again in order to fix playback whenever we dont hold the whole song in memory.
        // However this breaks playback using jack, since there (usually) resampling is necessary and calling JackOutput::write() with only e.g. 100 frames will not produce enough resampled frames that can be put into jack's playback buffer, padding its buffer with zeros, resulting in interrupted playback.
        //
        // thus the case when audioDriver.write() was only able to partly play the provided pcm chunk, cannot be recovered
        // but we can try it again whenever audioDriver failed at all (i.e. returned 0)
        if(this->isPlaying && framesWritten==0)
        {
            // something went terribly wrong, wait some time, so the cpu doesnt get too busy
            this_thread::sleep_for(chrono::milliseconds(1));
            // and try again
            goto again;
        }

        // ensure PCM buffer(s) are well filled
        this->currentSong->fillBuffer();
        
        if(framesWritten != framesToPush)
        {
            CLOG(LogLevel::INFO, "failed playing the rendered pcm chunk\nframes written: " << framesWritten << "\nframes pushed: " << framesToPush);
        }

        // update the playhead
        this->playhead+=framesWritten;
        // notify observers
        this->onPlayheadChanged.Fire(this->playhead);

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
        while(this->isPlaying)
        {
        this->audioDriver->start();
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
        this->_pause();
        // will be saved in futurePlayInternal, not sure if anybody will ever notice though
        throw;
    }
}

