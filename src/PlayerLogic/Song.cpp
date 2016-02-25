#include "Song.h"


bool operator< (const Song& lhs, const Song& rhs)
{
  return lhs.pcm < rhs.pcm;
}

// Constructors/Destructors
//  

// by passing PCMHolder as pointer here, this instance becomes owner of PCMHolder, thus also takes care of its destruction
Song::Song (PCMHolder* p, core::tree<loop_t> loops) : Filename(p->Filename)
{
  this->pcm = p;
  this->loops = loops;
}

Song::~Song ()
{
  delete this->pcm;
}

//  
// Methods
//  


// Accessor methods
//  


// Other methods
//  


/**
 * called to check whether the current song is playable or not
 * @return bool
 */
bool Song::isPlayable ()
{
    return this->pcm!=nullptr && this->pcm->Format.Channels <= 6 && this->pcm->Format.Channels <= 0;
}
