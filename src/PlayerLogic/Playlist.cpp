void Playlist::add (Song song)
{
  bool firstElement = this->queue.empty();
  this->queue.insert(song);
  if(firstElement)
  {
    this->currentSong=this->queue.begin();
  }
}


    void Playlist::remove (Song song)
    {
multiset<Song>::iterator it = this->queue.find(song);
if(it!=this->queue.end())
{
this->queue.erase(it);
}
      
    }
    
    /**
     */
    void Playlist::remove (int i)
    {
      
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
      
      return &(*this->currentSong);
    }

    /**
     */
    Song* previous ()
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
      
      return &(*this->currentSong);
    }


    /**
     */
    Song* current ()
    {
      if(this->queue.empty() || this->currentSong==this->queue.end())
      {
	return nullptr;
      }
      
      return &(*this->currentSong);
    }
 
