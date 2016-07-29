#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "Event.h"

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
  */

class Player
{
public:

    Player (IPlaylist* playlist);

    // forbid copying
    Player(Player const&) = delete;
    Player& operator=(Player const&) = delete;

    virtual ~Player ();

    /**
     */
    void play ();

    bool getIsPlaying();

    /**
     * @return Song
     */
    const Song* getCurrentSong ();


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
    void fadeout (unsigned int fadeTime);


    /**
     * @param  frame seeks the playhead to frame "frame"
     */
    void seekTo (frame_t frame);

    Event<frame_t> onPlayheadChanged;
    Event<> onCurrentSongChanged;


private:

    float PreAmpVolume = 1.0f;

    // pointer to the song we are currently playing
    // instance is owned by this.playlist
    Song* currentSong = nullptr;

    // pointer to the playlist we use
    // we dont own this playlist, we dont care about destruction
    IPlaylist* playlist = nullptr;

    // frame offset; (song.pcm + playhead*song.channels) points to the frame(s) that will be played on subsequent call to playSample
    frame_t playhead = 0;

    // pointer to the audioDriver, we currently use
    // we DO own this instance and should care about destruction
    IAudioOutput* audioDriver = nullptr;

    bool isPlaying = false;


    future<void> futurePlayInternal;


    void _initAudio();
    void _seekTo (frame_t frame);
    void _setCurrentSong (Song* song);
    void _pause ();

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
