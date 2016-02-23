#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <set>

class Playlist
{
public:

    // Constructors/Destructors
    //  
    // Empty virtual destructor for proper cleanup
    virtual ~Playlist()


    /**
     * @param  song
     */
    void add (Song song) override;


    /**
     */
    void remove (Song song) override;
    
    /**
     */
    void remove (int i) override;


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
  multiset<Song> queue;
  multiset<Song>::iterator currentSong = this->queue.begin();
};

#endif // PLAYLIST_H
