#ifndef PLAYLIST_H
#define PLAYLIST_H


#include "IPlaylist.h"

#include <queue>

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

    Song* getSong(unsigned int id) override;

    /**
     */
    Song* current () override;

protected:
    SongQueue_t queue;
    SongQueue_t::size_type currentSong = 0;
};

#endif // PLAYLIST_H
