#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
class MainWindow;
#include <future>

namespace core
{
template <typename T> class tree;
};
class IAudioOutput;
class IPlaylist;
class Song;

using namespace std;

/**
  * class Player
  *
  */

class Player
{
public:

    Player (IPlaylist* playlist);

    virtual ~Player ();

    /**
     */
    void play ();

    bool getIsPlaying();

    /**
     * @return Song
     */
    Song* getCurrentSong ();


    /**
     * @param  song
     */
    void setCurrentSong (Song* song);


    /**
     */
    void stop ();


    /**
     */
    void pause ();


    /**
     * prepares the player for the next song to be played, but doesnt start playback
     */
    void next ();


    /**
     */
    void previous ();


    /**
     */
    void fadeout ();


    /**
     * @param  frame seeks the playhead to frame "frame"
     */
    void seekTo (frame_t frame);
    void (*playheadChanged) (void*, frame_t) = nullptr;
    void (*currentSongChanged) (void*) = nullptr;
    void* callbackContext = nullptr;


private:

    float PreAmpVolume;

    // pointer to the song we are currently playing
    // instance is owned by this.playlist
    Song* currentSong = nullptr;

    // pointer to the playlist we use
    // we dont own this playlist, we dont care about destruction
    IPlaylist* playlist;

    // frame offset; (song.pcm + FRAMESTOFLOATS(playhead)) points to the frame that will be played on subsequent call to playSample
    frame_t playhead = 0;

    // pointer to the audioDriver, we currently use
    // we DO own this instance and should care about destruction
    IAudioOutput* audioDriver = nullptr;

    bool isPlaying = false;


    future<void> futurePlayInternal;


    void init();

    void _seekTo (frame_t frame);
    void _setCurrentSong (Song* song);

    /**
     * resets the playhead to beginning of currentSong
     */
    void resetPlayhead ();

    core::tree<loop_t>* getNextLoop(core::tree<loop_t>& l);

    /**
     * make sure you called seekTo(startFrame) before calling this!
     * @param  startFrame intended: the frame with which we want to start the playback
     *
     * actually: does nothing, just for debug purposes, the actual start is determined
     * by playhead
     * @param  stopFrame play until we've reached stopFrame, although this frame will
     * not be played
     */
    void playLoop (core::tree<loop_t>& loop);


    /**
     * @param  startFrame
     * @param  stopFrame
     */
    void playFrames (frame_t startFrame, frame_t stopFrame);


    /**
     * @param  itemsToPlay how many items (e.g. floats, not frames!!) from buffer shall
     * be played
     */
    void playFrames (frame_t framesToPlay);

    void playInternal ();

};

#endif // PLAYER_H
