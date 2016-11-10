#include "Song.h"

#include "CommonExceptions.h"

#include <algorithm> // std::sort

// Constructors/Destructors
//
Song::Song(string filename)
    : Filename(filename)
{
}

Song::Song(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen)
    : Filename(filename)
    , fileOffset(fileOffset)
    , fileLen(fileLen)
{
}


/**
  * called to check whether the current song is playable or not
  *
  * @return true if song can be played, else false
  */
bool Song::isPlayable () noexcept
{
    return this->Format.Channels <= 6 && this->Format.Channels > 0;
}


/**
  * calls Song::getLoopArray() and builds the loop tree
  */
void Song::buildLoopTree ()
{
    this->loopTree.clear();
    
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

/**
 * provide a default implementation so all formats, that dont support loops dont need to override this
 */
vector<loop_t> Song::getLoopArray () const noexcept
{
    vector<loop_t> res;
    return res;
}

/**
 * provide a default implementation so all formats, that dont have or support metadata dont need to override this
 */
void Song::buildMetadata() noexcept
{

}

// should sort descendingly
bool Song::myLoopSort(loop_t i,loop_t j)
{
    return (i.stop-i.start)>(j.stop-j.start);
}

/**
 * tells, whether "parent" can be a parent loop for "child"
 */
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
    // "child" should be the actual parent of "parent"
    // something went wrong
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

/**
 * within loopTree, recursively find the most fitting parent for subLoop
 */
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
