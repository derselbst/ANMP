#ifndef IPLAYLIST_H
#define IPLAYLIST_H

class Song;

/**
  * abstract base class IPlaylist for playlists
  *
  * Owns the songs to be played
  */

class IPlaylist
{
public:

    // Empty virtual destructor for proper cleanup
    virtual ~IPlaylist() = default;


    /**
     * @brief adds a song to the playlist
     *
     * appends a song to the playlist
     *
     * @param song the song to be added
     */
    virtual void add (Song* song) = 0;


    /**
     * @brief removes a song from the playlist
     *
     * searches for all occurrences of the given song and removes them
     *
     * @param song the song to be removed
     */
    virtual void remove (Song* song) = 0;

    /**
     * @brief removes an element at the given index
     *
     * @param idx a zero-based index, specifying the element to be removed
     */
    virtual void remove (int idx) = 0;

    /**
     * @brief clears the playlist
     *
     * removes all elements from the playlist
     */
    virtual void clear() = 0;

    /**
     * @brief gets the song at the given index
     *
     * @param idx a zero-based index, specifying the element to be retrieved
     *
     * @return returns the song at the specified index, nullptr if song does not exist
     */
    virtual Song* getSong(unsigned int idx) const = 0;


    /**
     * @brief sets the next song to play
     *
     * sets the song that follows the currently played back song as new currently played back song
     *
     * @return returns the new currently played back song, nullptr if there is no next song, which makes playback stop
     */
    virtual Song* next () = 0;


    /**
     * @brief sets the previous song to play
     *
     * sets the song that precedes the currently played back song as new currently played back song
     *
     * @return returns the new currently played back song, nullptr if there is no previous song, which makes playback stop
     */
    virtual Song* previous () = 0;


    /**
     * @brief gets the currently played back song
     *
     * @return returns the currently played back song, nullptr if there is no song set, which makes playback stop
     */
    virtual Song* current () = 0;

    /**
     * @brief sets the song currently played back
     *
     * the song at the givend index becomes the currently played song
     *
     * @param idx a zero-based index, specifying the element to be set for current playback
     *
     * @return returns the song at the specified index (same as this->getSong(idx))
     */
    virtual Song* setCurrentSong(unsigned int idx) = 0;

};

#endif // IPLAYLIST_H
