#include "Playlist.h"

#include <algorithm>


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
    this->queue.erase(std::remove(this->queue.begin(), this->queue.end(), song), this->queue.end());
}

/**
 */
void Playlist::remove (int i)
{
  if(this->queue.empty()) { return; }
  
    this->queue.erase(this->queue.begin()+(i % this->queue.size()));
}

void Playlist::clear()
{
    this->queue.clear();
}


/**
 */
Song* Playlist::current ()
{
    return this->getSong(this->currentSong);
}

/**
 */
Song* Playlist::next ()
{
    return this->setCurrentSong((this->currentSong+1) % this->queue.size());
}

/**
 */
Song* Playlist::previous ()
{
    return this->setCurrentSong((this->currentSong+this->queue.size()-1) % this->queue.size());
}

Song* Playlist::getSong(unsigned int id)
{
    if(this->queue.size() > id)
    {
        return this->queue[id];
    }

    return nullptr;
}

Song* Playlist::setCurrentSong(unsigned int id)
{
    Song* s = this->getSong(id);
    if(s!=nullptr)
    {
      this->currentSong = id;
    }
    
    return s;
}
