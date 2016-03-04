#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <queue>

#include "IPlaylist.h"

class Playlist : public IPlaylist
{
public:
    typedef deque<Song*> SongQueue_t;

    // Constructors/Destructors
    //
    // Empty virtual destructor for proper cleanup
    virtual ~Playlist();


    /**
     * @param  song
     */
    void add (Song* song) override;


    /**
     */
    void remove (Song* song) override;

    /**
     */
    void remove (int i) override;

    void clear() override;

    /**
     */
    Song* next () override;

    /**
     */
    Song* previous () override;


    /**
     */
    Song* current () override;

private:
    SongQueue_t queue;
    SongQueue_t::iterator currentSong = this->queue.begin();
};

#endif // PLAYLIST_H
