#include "Playlist.h"

#define EMPTY_GUARD     if(this->queue.empty()) { return nullptr; }

Playlist::~Playlist()
{
    this->clear();
}

void Playlist::add (Song* song)
{
    this->queue.push_back(song);
}


void Playlist::remove (Song* song)
{
    for(SongQueue_t::iterator it = this->queue.begin(); it!=this->queue.end(); it++)
    {
        if(*it == song)
        {
            delete *it;
            this->queue.erase(it);
        }
    }
}

/**
 */
void Playlist::remove (int i)
{
    this->remove(this->queue[i]);
}

void Playlist::clear()
{
    this->queue.clear();
}


/**
 */
Song* Playlist::next ()
{
    EMPTY_GUARD

    this->currentSong = (this->currentSong+1) % this->queue.size();

    return this->queue[this->currentSong];
}

/**
 */
Song* Playlist::previous ()
{
    EMPTY_GUARD

    this->currentSong = (this->currentSong+this->queue.size()-1) % this->queue.size();

    return this->queue[this->currentSong];
}

Song* Playlist::getSong(unsigned int id)
{
    if(this->queue.size() > id)
    {
        // TODO: this is a simple getter function, that should NOT implicitly set id
        // as currentSong
        this->currentSong = id;

        return this->queue[id];
    }

    return nullptr;
}

/**
 */
Song* Playlist::current ()
{
    EMPTY_GUARD

    return this->queue[this->currentSong];
}

