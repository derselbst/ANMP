#ifndef PLAYLIST_H
#define PLAYLIST_H


#include "IPlaylist.h"

#include <queue>
#include <mutex>

using namespace std;

class Playlist : public IPlaylist
{
public:
    typedef deque<Song*> SongQueue_t;

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
    Song* current () override;

    /**
     */
    Song* next () override;

    /**
     */
    Song* previous () override;

    Song* getSong(unsigned int id) override;

    Song* setCurrentSong(unsigned int id) override;

    void move(signed int source, unsigned int count, int steps);


protected:
    SongQueue_t queue;
    SongQueue_t::size_type currentSong = 0;

    mutable recursive_mutex mtx;
};

#endif // PLAYLIST_H
