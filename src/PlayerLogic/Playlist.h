#ifndef PLAYLIST_H
#define PLAYLIST_H


#include "IPlaylist.h"

#include <mutex>
#include <queue>
#include <random>


class Playlist : public IPlaylist
{
    public:
    typedef std::deque<Song *> SongQueue_t;

    Playlist();
    virtual ~Playlist();


    size_t add(Song *song) override;

    void remove(Song *song) override;

    void remove(size_t i) override;

    void clear() override;

    Song *getCurrentSong() override;
    size_t getCurrentSongId() override;
    size_t size();

    Song *next() override;

    Song *previous() override;

    Song *getSong(size_t id) const override;

    Song *setCurrentSong(size_t id) override;

    void move(size_t source, unsigned int count, int steps);

    virtual void shuffle(size_t start, size_t end);


    protected:
    SongQueue_t queue;
    SongQueue_t::size_type currentSong = 0;

    std::random_device rd;
    std::mt19937 urbg;

    // synchronize concurrent access made by playback thread and qt's gui thread
    mutable std::recursive_mutex mtx;
};

#endif // PLAYLIST_H
