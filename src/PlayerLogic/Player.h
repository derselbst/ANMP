#ifndef PLAYER_H
#define PLAYER_H

#include <future>

#include "Song.h"
#include "IPlaylist.h"
#include "IAudioOutput.h"

/**
  * class Player
  *
  */

class Player
{
public:

    // Constructors/Destructors
    //


    /**
     * Empty Constructor
     */
    Player (IPlaylist* playlist);

    /**
     * Empty Destructor
     */
    virtual ~Player ();

    // Static Public attributes
    //

    // Public attributes
    //

    void init();

    /**
     */
    void play ();


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


private:

    // Static Private attributes
    //

    // Private attributes
    //

    float PreAmpVolume;

    Song* currentSong = nullptr;
    recursive_mutex mtxCurrentSong;

    IPlaylist* playlist;

    // frame offset; (song.pcm + FRAMESTOFLOATS(playhead)) points to the frame that will be played on subsequent call to playSample
    frame_t playhead = 0;

    IAudioOutput* audioDriver = nullptr;

    bool isPlaying = false;

    future<void> futureFillBuffer;
    future<void> futurePlayInternal;


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
    void playFrames (unsigned int itemsToPlay);

    void playInternal ();

};

#endif // PLAYER_H
