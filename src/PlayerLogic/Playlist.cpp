#include "Playlist.h"

Playlist::~Playlist()
{
this->clear();
}

void Playlist::add (Song* song)
{
    bool firstElement = this->queue.empty();
    this->queue.push_back(song);
    if(firstElement)
    {
        this->currentSong=this->queue.begin();
    }
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
    if(this->queue.empty())
    {
        return nullptr;
    }

    if(this->currentSong==this->queue.end() || this->currentSong==--this->queue.end())
    {
        // if we reached end of queue, start at beginning
        this->currentSong = this->queue.begin();
    }
    else
    {
        this->currentSong++;
    }

    return *this->currentSong;
}

/**
 */
Song* Playlist::previous ()
{
    if(this->queue.empty())
    {
        return nullptr;
    }

    if(this->currentSong==this->queue.begin())
    {
        // if we are already at beginning of queue, return last song
        this->currentSong=--this->queue.end();
    }
    else
    {
        this->currentSong--;
    }

    return *this->currentSong;
}


/**
 */
Song* Playlist::current ()
{
    if(this->queue.empty() || this->currentSong==this->queue.end())
    {
        return nullptr;
    }

    return *this->currentSong;
}

