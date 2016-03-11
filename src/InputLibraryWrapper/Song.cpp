#include "Song.h"

#include "CommonExceptions.h"

#include <algorithm>

// Constructors/Destructors
//

Song::Song(string filename, size_t fileOffset, size_t fileLen)
{
    this->Filename = filename;
    this->fileOffset = fileOffset;
    this->fileLen = fileLen;    
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

bool operator < (loop_t const& lhs, loop_t const& rhs)
{
  return lhs.start < rhs.start && lhs.stop <= rhs.start;
}
bool operator == (loop_t const& lhs, loop_t const& rhs)
{
  return lhs.start == rhs.start &&
         lhs.stop  == rhs.stop  &&
         lhs.count == rhs.count &&
         lhs.type  == rhs.type;
}


/**
 * @return core::tree
 * @param  p
 */
void Song::buildLoopTree ()
{
    loop_t root;
    root.start = 0;
    root.stop = this->getFrames();
    root.count = 1;
    *this->loopTree = root;
    
      vector<loop_t> loopvec = this->getLoopArray();

      // TODO: make sure loop array is sorted correctly
      std::sort (loopvec.begin(), loopvec.end(), myLoopSort);
      for(vector<loop_t>::iterator it = loopvec.begin(); it!=loopvec.end(); it++)
      {
        core::tree<loop_t>& subNode = findRootLoopNode(this->loopTree, *it);
        subNode.insert(*it);
      }
}

vector<loop_t> Song::getLoopArray () const
{
    vector<loop_t> res;
    return res;
}

// should sort descendingly
bool Song::myLoopSort(loop_t i,loop_t j)
{
    return (i.stop-i.start)>(j.stop-j.start);
}

bool Song::loopsMatch(const loop_t& parent, const loop_t& child)
{
  // safety check
  if(parent.start >= parent.stop || child.start >= child.stop)
  {
    throw LoopTreeConstructionException();
  }
  
  // start and stop of child are within bounds of parent
  if((parent.start <= child.start && child.start < parent.stop) &&
     (parent.start <  child.stop  && child.stop <= parent.stop))
  {
    return true;
  }
  // child loop should be actual parent of parent
  if(child.start < parent.start && parent.stop < child.stop)
  {
    throw LoopTreeImplementationException();
  }
    
  // start and stop of child are either smaller parent.start or bigger parent.stop
  if((child.start < parent.start && child.stop <= parent.start) || (child.start >= parent.stop && child.stop > parent.stop))
  {
    return false;
  }
    
  // loops are crossing each other
  if((parent.start <= child.start && child.start < parent.stop && parent.stop < child.stop) || // child.start within parent but not child.stop
     (child.start < parent.start  && parent.start< child.stop  && child.stop  < parent.stop))  // child.stop within parent but not child.start
  {
    throw LoopTreeConstructionException();
  }
  
  // we should never get here
  throw NotImplementedException();
}

core::tree<loop_t>& Song::findRootLoopNode(core::tree<loop_t>& loopTree, const loop_t& subLoop)
{
  for (core::tree<loop_t>::iterator iter = loopTree.begin(); iter != loopTree.end(); ++iter)
  {
    if(loopsMatch(*iter, subLoop))
    {
      return findRootLoopNode(iter.tree_ref(), subLoop);
    }
  }
  
  return loopTree;
}
