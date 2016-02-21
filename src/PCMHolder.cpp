#include "PCMHolder.h"

// Constructors/Destructors
//  

PCMHolder::PCMHolder () {
initAttributes();
}

PCMHolder::~PCMHolder () { }

//  
// Methods
//  


// Accessor methods
//  


// Other methods
//  


/**
 * opens the current file using the corresponding lib
 */
void PCMHolder::open ()
{
}


/**
 */
void PCMHolder::close ()
{
}


/**
 * @param  items ensure that the buffer holds at least "items" floats
 */
void PCMHolder::fillBuffer (unsigned int items)
{
// synchronos!! read call to library goes here
}


/**
 */
void PCMHolder::releaseBuffer ()
{
}


/**
 * @return vector
 */
vector PCMHolder::getLoops () const
{
}


/**
 * returns number of frames that are (expected to be) in pcm buffer
 * @return unsigned int
 */
unsigned int PCMHolder::getFrames () const
{
}

void PCMHolder::initAttributes () {
    data = nullptr;
    count = 0;
    offset = 0;
}

