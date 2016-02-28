#include "Song.h"

// Constructors/Destructors
//

Song::Song ()
{
  
}

Song::~Song ()
{
  
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
    return this->Format.Channels <= 6 && this->Format.Channels > 0;
}



/**
 * @return core::tree
 * @param  p
 */
void Song::buildLoopTree ()
{
    loop_t l;
    l.start = 0;
    l.stop = this->getFrames();
    l.count = 1;
    *this->loopTree = l;
    /* TODO:implement!!
      vector<loop_t> l = p.getLoops();

      // TODO: make sure loop array is sorted correctly
      std::sort (l.begin(), l.end(), myLoopSort);
      foreach loop in l
      {
        core::tree subNode = findRootLoopNode(loopTree, loop);
        subNode.insert();
      }*/
}



// should sort descendingly
bool myLoopSort(loop_t i,loop_t j)
{
    return (i.stop-i.start)>(j.stop-j.start);
}
