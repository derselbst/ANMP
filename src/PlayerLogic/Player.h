#ifndef PLAYER_H
#define PLAYER_H


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
    Player ();

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
    Song getCurrentSong ();


    /**
     * @param  song
     */
    void setCurrentSong (Song& song);


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
    void seekTo (unsigned int frame);

    
private:

    // Static Private attributes
    //  

    // Private attributes
    //  

    float PreAmpVolume;
    Song* currentSong = nullptr;
    IPlaylist playlist;
    // frame offset; (song.pcm + offset + FRAMESTOFLOATS(playhead)) points to the frame that will be played on subsequent call to playSample
    size_t playhead = 0;
    IAudioOutput* audioDriver;
    bool isPlaying = false;
    
    future<void> futureFillBuffer;



    /**
     * resets the playhead to beginning of currentSong
     */
    void resetPlayhead ();


    /**
     * make sure you called seekTo(startFrame) before calling this!
     * @param  startFrame intended: the frame with which we want to start the playback
     * 
     * actually: does nothing, just for debug purposes, the actual start is determined
     * by playhead
     * @param  stopFrame play until we've reached stopFrame, although this frame will
     * not be played
     */
    void playLoop (unsigned int startFrame, unsigned int stopFrame);


    /**
     * @param  startFrame
     * @param  stopFrame
     */
    void playFrames (unsigned int startFrame, unsigned int stopFrame);


    /**
     * @param  itemsToPlay how many items (e.g. floats, not frames!!) from buffer shall
     * be played
     */
    void playFrames (unsigned int itemsToPlay);

    void playInternal ();

};

#endif // PLAYER_H
