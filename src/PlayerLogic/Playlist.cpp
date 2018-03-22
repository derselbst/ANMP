#include "Playlist.h"
#include "Song.h"

#include <algorithm>


Playlist::~Playlist()
{
    this->clear();
}

size_t Playlist::add(Song *song)
{
    lock_guard<recursive_mutex> lck(this->mtx);

    this->queue.push_back(song);

    return this->queue.size() - 1;
}


void Playlist::remove(Song *song)
{
    lock_guard<recursive_mutex> lck(this->mtx);

    this->queue.erase(std::remove(this->queue.begin(), this->queue.end(), song), this->queue.end());
    delete song;
}

void Playlist::remove(size_t i)
{
    lock_guard<recursive_mutex> lck(this->mtx);

    if (this->queue.empty())
    {
        return;
    }

    SongQueue_t::iterator it = this->queue.begin() + (i % this->queue.size());
    Song *s = *it;
    this->queue.erase(it);
    delete s;

    if (i < this->currentSong)
    {
        this->currentSong--;
    }
}

void Playlist::clear()
{
    lock_guard<recursive_mutex> lck(this->mtx);

    Song *s;
    for (SongQueue_t::iterator it = this->queue.begin(); it != this->queue.end(); ++it)
    {
        s = *it;
        delete s;
    }

    this->queue.clear();
    this->currentSong = 0;
}

Song *Playlist::getCurrentSong()
{
    lock_guard<recursive_mutex> lck(this->mtx);

    return this->getSong(this->currentSong);
}

size_t Playlist::getCurrentSongId()
{
    lock_guard<recursive_mutex> lck(this->mtx);

    return this->currentSong;
}

Song *Playlist::next()
{
    lock_guard<recursive_mutex> lck(this->mtx);

    if (this->queue.empty())
    {
        return nullptr;
    }

    return this->setCurrentSong((this->currentSong + 1) % this->queue.size());
}

Song *Playlist::previous()
{
    lock_guard<recursive_mutex> lck(this->mtx);

    if (this->queue.empty())
    {
        return nullptr;
    }

    return this->setCurrentSong((this->currentSong + this->queue.size() - 1) % this->queue.size());
}

Song *Playlist::getSong(size_t id) const
{
    lock_guard<recursive_mutex> lck(this->mtx);

    if (this->queue.size() > id)
    {
        return this->queue[id];
    }

    return nullptr;
}

Song *Playlist::setCurrentSong(size_t id)
{
    lock_guard<recursive_mutex> lck(this->mtx);

    Song *s = this->getSong(id);
    if (s != nullptr)
    {
        this->currentSong = id;
    }

    return s;
}

void Playlist::shuffle(size_t start, size_t end)
{
    if (start > end)
    {
        throw invalid_argument("start>end");
    }

    lock_guard<recursive_mutex> lck(this->mtx);

    if (start <= this->currentSong && this->currentSong <= end)
    {
        if (start - this->currentSong != 0)
        {
            std::random_shuffle(this->queue.begin() + start, this->queue.begin() + this->currentSong);
        }

        if (this->currentSong + 1 - end != 0)
        {
            std::random_shuffle(this->queue.begin() + (this->currentSong + 1), this->queue.begin() + end);
        }
    }
    else
    {
        std::random_shuffle(this->queue.begin() + start, this->queue.begin() + end);
    }
}

/** @brief move songs within the playlist
 *
 * rotates songs selected by [source, source+count] "steps" steps
 *
 * @param[in] source zero-based index indicating the start of the selection
 * @param[in] count number of elements to follow after source
 * @param[in] steps if positive: move them "steps" steps further to the end of the queue
 *                  if negative: move them "steps" steps further to the start of the queue
 */
void Playlist::move(size_t source, unsigned int count, int steps)
{
    lock_guard<recursive_mutex> lck(this->mtx);

    SongQueue_t &que = this->queue;

    if (source < 0 || que.size() < source + count)
    {
        return;
    }

    if (steps < 0) // left shift
    {
        std::rotate(source + steps >= 0 ?
                    std::next(que.begin(), source + steps) :
                    que.begin(),
                    std::next(que.begin(), source),
                    std::next(que.begin(), source + count + 1));

        // update currentSong
        if (source <= this->currentSong && this->currentSong <= source + count)
        {
            // currentSong is in the selection [source, source+count]
            // it has moved along with the rotate call
            this->currentSong += steps; // is subtraction!!
        }
        else if (source + steps <= this->currentSong && this->currentSong <= source)
        {
            // currentSong was not selected, but it intersected the move
            this->currentSong += (-steps) + count;
        }
    }
    else if (steps > 0) // right shift
    {
        std::rotate(std::next(que.begin(), source),
                    std::next(que.begin(), source + count + 1),
                    source + count + steps < que.size() ?
                    std::next(que.begin(), source + count + steps + 1) :
                    que.end());

        // update currentSong
        if (source <= this->currentSong && this->currentSong <= source + count)
        {
            // current song is in the selection [source, source+count]
            // it has moved along with the rotate call
            this->currentSong += steps;
        }
        else if (source + count < this->currentSong && this->currentSong <= source + count + steps)
        {
            // currentSong was not selected, but it intersected the move
            this->currentSong -= steps + count;
        }
    }
}

size_t Playlist::size()
{
    return this->queue.size();
}
