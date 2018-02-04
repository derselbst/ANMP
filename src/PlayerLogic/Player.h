#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "Event.h"

#include <atomic>
#include <future>

namespace core
{
template <typename T> class tree;
}
class IAudioOutput;
class IPlaylist;
class Song;

using namespace std;

/**
  * class Player
  *
  * contains logic for playing back a Song
  */

class Player
{
public:

    Player (IPlaylist* playlist);

    // forbid copying
    Player(Player const&) = delete;
    Player& operator=(Player const&) = delete;
    
    Player(Player&& other);
    
    virtual ~Player ();
    
    /**
     * initializes the audio driver. usually called automatically if required.
     * 
     * the audio driver to be used is indicated by gConfig.audioDriver
     */
    void initAudio();

    /**
     * resets the playback volume to this->PreAmpVolume and starts the playback
     * 
     * if(this->currentSong==nullptr) method returns without playback being started
     */
    void play ();
    
    /**
     * stops the playback, returning after the internal playthread exited
     */
    void pause ();
    
    /**
     * same as this->pause(), but also rewinds this->playhead
     */
    void stop ();
    
    /**
     * @return true, if currently playing back, else false
     */
    bool IsPlaying();
    
    /**
     * @return true, if seeking withing the currently played song is possible
     */
    bool IsSeekingPossible();

    /**
     * smoothly decreases playback volume to zero. when this method returns, the volume reached zero and playback continues!
     * 
     * @param fadeTime time in milliseconds needed to decrease the volume
     * @param fadeType specifies the fade type to use: 1 - linear; 2 - log; 3 - sine;
     */
    void fadeout (unsigned int fadeTime, int8_t fadeType=3);

    /**
     * @return const representation of the currently played song
     */
    const Song* getCurrentSong ();

    /**
     * sets the song currently being played. for this, the playback gets stopped, playhead gets rewinded and "song" gets set.
     * 
     * the original playback state gets restored, if "song"!=nullptr
     * 
     * @param song pointer to the song being played after this method returned
     */
    void setCurrentSong (Song* song);

    /**
     * @param  frame seeks the playhead to frame "frame"
     */
    void seekTo (frame_t frame);

    void Mute(int i, bool);
    
    Event<bool, Nullable<string>> onIsPlayingChanged;
    Event<frame_t> onPlayheadChanged;
    Event<const Song*> onCurrentSongChanged;


private:

    float PreAmpVolume = 1.0f;

    // pointer to the song we are currently playing
    // instance is owned by this.playlist
    Song* currentSong = nullptr;

    // pointer to the playlist we use
    // we dont own this playlist, we dont care about destruction
    IPlaylist* playlist = nullptr;

    // frame offset; (currentSong.data + playhead*currentSong.Format.Channels) points to the frame(s) that will be played on subsequent call to playFrames(frame_t)
    atomic<frame_t> playhead{0};

    // pointer to the audioDriver, we currently use
    // we DO own this instance and should care about destruction
    IAudioOutput* audioDriver = nullptr;

    // are we currently playing back?
    atomic<bool> isPlaying{false};

    // future for the playing thread
    future<void> futurePlayInternal;


    /**
     * private methods containing the acutal implementation logic for their corresponding public ones
     */
    void _initAudio();
    void _seekTo (frame_t frame);
    void _setCurrentSong (Song* song);
    void _pause ();
    
    /**
     * within this->currentSong->loopTree at a level given by "l": retrieve that loop that starts just right after playhead
     * 
     * @return subloop as subnode
     */
    core::tree<loop_t>* getNextLoop(core::tree<loop_t>& l);


    /**
     * THAT method which recursively walks through a loop tree, playing all the loops, subloops and subsubsubloops given by "loop" recursively
     *
     * @note this is not the playing threads mainloop
     * @note this method might be called recursively :P
     */
    void playLoop (core::tree<loop_t>& loop);


    /**
     * plays a loop with the bounds specified by startFrame and stopFrame.
     * starts playing at whereever playhead stands.
     * returns as soon as playhead leaves the bounds, i.e. exceeding stopFrame or underceeding startFrame.
     *
     * @param  startFrame zero-based array index == the lower bound this->playhead shall be in
     * @param  stopFrame zero-based array index == the upper bound this->playhead shall be in, i.e. play until we've reached stopFrame, although the frame at "stopFrame" will not be played.
     *
     * @todo really ensure and test that this last frame is not being played
     */
    void playFrames (frame_t startFrame, frame_t stopFrame);


    /**
     * called by playFrames(frame_t, frame_t)
     * 
     * play "framesToPlay" frames from the current position (indicated by this->playhead)
     *
     * it ought to start playing that frame, which is currently pointed to by this->playhead
     *
     * @param framesToPlay no. of frames to play from the current position
     */
    void playFrames (frame_t framesToPlay);

    /**
     * the internal loop for the playing thread
     */
    void playInternal ();

};

#endif // PLAYER_H
