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
#define FramesToItems(x) ((x)*this->currentSong->Format.Channels())
#define USERS_ARE_STUPID if(this->audioDriver==nullptr || this->playlist==nullptr || this->currentSong==nullptr){throw NotInitializedException();}
// Constructors/Destructors
//

Player::Player (IPlaylist* playlist)
{
    this->playlist = playlist;
}

Player::Player (Player&& other)
{
    other.pause();
  
    this->currentSong = other.currentSong;
    other.currentSong = nullptr;
  
    this->playlist = other.playlist;
    other.playlist = nullptr;
    
    this->audioDriver = other.audioDriver;
    other.audioDriver = nullptr;
    
    this->playhead.store(other.playhead.load());
    this->PreAmpVolume = other.PreAmpVolume;
}

Player::~Player ()
{
    CLOG(LogLevel_t::Debug, "destroy player " << hex << this);
    this->pause();

    delete this->audioDriver;
}

void Player::initAudio()
{
    bool oldState = this->IsPlaying();
    this->pause();
    this->_initAudio();

    if(oldState)
    {
        this->play();
    }
}

void Player::_initAudio()
{
    this->_pause();

    delete this->audioDriver;

    switch(gConfig.audioDriver)
    {
#ifdef USE_ALSA
    case AudioDriver_t::Alsa:
        this->audioDriver = new ALSAOutput();
        break;
#endif
#ifdef USE_EBUR128
    case AudioDriver_t::Ebur128:
        this->audioDriver = new ebur128Output(this);
        break;
#endif
#ifdef USE_JACK
    case AudioDriver_t::Jack:
        this->audioDriver = new JackOutput();
        break;
#endif
    case AudioDriver_t::Wave:
        this->audioDriver = new WaveOutput(this);
        break;
#ifdef USE_PORTAUDIO
    case AudioDriver_t::Portaudio:
        this->audioDriver = new PortAudioOutput();
        break;
#endif
    default:
        this->audioDriver=nullptr;
        throw NotImplementedException();
        break;
    }

    this->audioDriver->open();

    if(this->currentSong!=nullptr)
    {
        this->audioDriver->init(this->currentSong->Format);
    }
}

bool Player::IsPlaying()
{
    return this->isPlaying;
}

bool Player::IsSeekingPossible()
{
    return (this->currentSong==nullptr ? false : this->currentSong->count==FramesToItems(static_cast<size_t>(this->currentSong->getFrames())));
}

void Player::play ()
{
    if (this->IsPlaying())
    {
        return;
    }

    WAIT(this->futurePlayInternal);

    if(this->currentSong==nullptr)
    {
        // maybe we are uninitialized. ask playlist to be sure
        Song* s=this->playlist->current();
        if(s==nullptr)
        {
            return;
        }
        else
        {
            this->setCurrentSong(s);
        }
    }

    this->audioDriver->setVolume(this->PreAmpVolume);

    this->isPlaying=true;

    // new thread that runs playInternal
    this->futurePlayInternal = async(launch::async, &Player::playInternal, this);
}


const Song* Player::getCurrentSong ()
{
    return this->currentSong;
}


void Player::setCurrentSong (Song* song)
{
    if(this->IsPlaying())
    {
        throw runtime_error("Player: Cannot set song while still playing back.");
    }

    WAIT(this->futurePlayInternal);
    this->_setCurrentSong(song);
}

// this method is quite unlinear, but unfortunately it seems to be the only correct way to do it
// basically currentSong and newSong have to be opened while audioDriver is inited and callback is done
void Player::_setCurrentSong (Song* newSong)
{
    // make sure audio driver is initialized
    if(this->audioDriver==nullptr)
    {
        // if successfull this->audioDriver will be != nullptr and opened
        // if not: exception will be thrown
        this->_initAudio();
    }
        
    this->resetPlayhead();
    
    Song* oldSong = this->currentSong;
    try
    {
        if(newSong == nullptr) // nullptr here means this.stop()
        {
            this->_pause();
        }
        else if(oldSong == nullptr)
        {
            newSong->open();
            newSong->fillBuffer();
            this->audioDriver->init(newSong->Format);
        }
        else
        {   
            if(newSong == oldSong)
            {
                oldSong->releaseBuffer();
                
                // hard close and reopen the file, since might have changed
                oldSong->close();
                oldSong->open();
                oldSong->fillBuffer();
            }
            else
            {
                oldSong->releaseBuffer();
                newSong->open();
                newSong->fillBuffer();
                // DO NOT CLOSE oldSong here!
                // some audiodrivers (waveoutput) might be doing some calls to the old song which are only valid if the song is still open
            }

            this->audioDriver->init(newSong->Format);
        }
        
        // then update currently played song
        this->currentSong = newSong;

        // now we are ready to do the callback
        this->onCurrentSongChanged.Fire();
        
        // oldSong needs to stay open until the very end, i.e. after onCurrentSongChanged.Fire() and audioDriver init() are done!
        if(oldSong != nullptr && oldSong != newSong)
        {
            oldSong->releaseBuffer();
            oldSong->close();
        }
    }
    catch(const exception& e)
    {
        // cleanup
        if(newSong != nullptr)
        {
            newSong->releaseBuffer();
            newSong->close();
        }
        
        if(oldSong != nullptr)
        {
            oldSong->releaseBuffer();
            oldSong->close();
        }
        
        this->currentSong = nullptr;
        
        throw;
    }
}

void Player::stop ()
{
    this->pause();
    this->resetPlayhead();
}

void Player::pause ()
{
    this->_pause();
    WAIT(this->futurePlayInternal);
}

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

void Player::next ()
{
    this->setCurrentSong(this->playlist->next());
}

void Player::previous ()
{
    this->setCurrentSong(this->playlist->previous());
}

void Player::fadeout (unsigned int fadeTime, int8_t fadeType)
{
    USERS_ARE_STUPID

    if(fadeTime == 0)
    {
        this->audioDriver->setVolume(0);
    }

    float vol = 0.0f;
    for(float timePast=0.0; timePast <= fadeTime; timePast++)
    {
        switch (fadeType)
        {
        case 1:
            // linear
            vol =  1.0f - (timePast / fadeTime);
            break;
        case 2:
            // logarithmic
            vol = 1.0f - pow(0.1, (1 - (timePast / fadeTime)) * 1);
            break;
        case 3:
            // sine
            vol =  1.0f - sin( (timePast / fadeTime) * M_PI / 2 );
            break;
        }

        float volToPush = vol*this->PreAmpVolume;
        this->audioDriver->setVolume(volToPush);
        this_thread::sleep_for(chrono::milliseconds(1));
    }
}

void Player::Mute(int i, bool isMuted)
{
    this->currentSong->Format.VoiceIsMuted[i] = isMuted;
}

void Player::seekTo (frame_t frame)
{
    if(this->IsSeekingPossible())
    {
        this->_seekTo(frame);
    }
}

void Player::_seekTo (frame_t frame)
{
    if(frame < 0)
    {
        // negative frame not allowed for playhead, silently set it to 0
        frame = 0;
    }
    else if(this->currentSong != nullptr && frame >= this->currentSong->getFrames())
    {
        return;
    }

    this->playhead=frame;
}

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

    while(this->IsPlaying() && // are we still playing?
            gConfig.useLoopInfo && // user wishes to use available loop info
            this->IsSeekingPossible() && // we loop by setting the playhead, if this is not possible, since we dont hold the whole pcm, no loops are available
            ((subloop = this->getNextLoop(loop)) != nullptr)) // are there subloops left that need to be played?
    {

        // if the user requested to seek past the current loop, skip it at all
        if(this->playhead > (*(*subloop)).stop)
        {
            cerr << "this code seems to be redundant... NO IT ISNT!!!" << endl;
            continue;
        }

        this->playFrames((*loop).start, (*(*subloop)).start);
        // at this point: playhead==subloop.start

        uint32_t mycount = gConfig.overridingGlobalLoopCount!=-1 ? gConfig.overridingGlobalLoopCount : (*(*subloop)).count;
        bool forever = mycount==0;
        mycount += 1; // +1 because the subloop we are just going to play, should be played one additional time by the parent of subloop (i.e. the loop we are currently in)
        while(this->IsPlaying() && (forever || mycount-- != 0u))
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
    this->playFrames((*loop).start, (*loop).stop);
}


void Player::playFrames (frame_t startFrame, frame_t stopFrame)
{
    USERS_ARE_STUPID

    // the user may request to seek while we are playing, thus check whether playhead is
    // still in range
    while(this->IsPlaying() && this->playhead>=startFrame && this->playhead<stopFrame)
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
    while(this->IsPlaying() && // has smb. stopped playback?
            framesToPlay>0 && // are there any frames left to play?
            bufSize!=0 && // are there any samples in pcm buffer?
            memorizedPlayhead==this->playhead // make sure noone seeked while playing
         )
    {
        // seek within the pcm buffer to that item where the playhead points to, but make sure we dont run over the buffer; in doubt we should start again at the beginning of the buffer
        int itemOffset = FramesToItems(memorizedPlayhead) % bufSize;
        // number of frames we will write to audioDriver in this run
        int framesToPush = min(gConfig.FramesToRender, framesToPlay);

        int framesWritten = 0;

        // PLAY!
again:
//         this->audioDriver->SetVoiceConfig(this->currentSong->Format.Voices, this->currentSong->Format.VoiceChannels);
        this->audioDriver->SetMuteMask(this->currentSong->Format.VoiceIsMuted);
        
        framesWritten = this->audioDriver->write(this->currentSong->data, framesToPush, itemOffset);
        // before we go on rendering the next pcm chunk, make sure we really played the current one.
        //
        // audioDriver.write() may return a value !=framesToPush.
        // In this case we should call audioDriver.write() again in order to fix playback whenever we dont hold the whole song in memory.
        // However this breaks playback using jack, since there (usually) resampling is necessary and calling JackOutput::write() with only e.g. 100 frames will not produce enough resampled frames that can be put into jack's playback buffer, padding its buffer with zeros, resulting in interrupted playback.
        //
        // thus the case when audioDriver.write() was only able to partly play the provided pcm chunk, cannot be recovered
        // but we can try it again whenever audioDriver failed at all (i.e. returned 0)
        if(this->IsPlaying() && framesWritten==0)
        {
            // something went terribly wrong, wait some time, so the cpu doesnt get too busy
            this_thread::sleep_for(chrono::milliseconds(1));
            // and try again
            goto again;
        }

        // ensure PCM buffer(s) are well filled
        this->currentSong->fillBuffer();

        if(framesWritten != framesToPush
#ifdef USE_JACK
            && gConfig.audioDriver != AudioDriver_t::Jack /*very spammy for jack*/
#endif      
        )
        {
            CLOG(LogLevel_t::Info, "failed playing the rendered pcm chunk\nframes written: " << framesWritten << "\nframes pushed: " << framesToPush);
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
    Nullable<string> exceptionMsg = Nullable<string>();
    this->onIsPlayingChanged.Fire(this->IsPlaying(), exceptionMsg);

    try
    {
        this->audioDriver->start();
        while(this->IsPlaying())
        {
            core::tree<loop_t>& loops = this->currentSong->loopTree;

            this->playLoop(loops);

            // ok, for some reason we left playLoop, if this was due to we shall stop playing
            // leave this loop immediately, else play next song
            if(!this->IsPlaying())
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
        exceptionMsg = e.what();
    }

    this->onIsPlayingChanged.Fire(this->IsPlaying(), exceptionMsg);
}

