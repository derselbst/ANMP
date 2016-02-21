
#ifndef PCMHOLDER_H
#define PCMHOLDER_H

#include <string>

/**
  * class PCMHolder
  * 
  */

class PCMHolder
{
public:

    // Constructors/Destructors
    //  


    /**
     * Empty Constructor
     */
    PCMHolder ();

    /**
     * Empty Destructor
     */
    virtual ~PCMHolder ();

    // Static Public attributes
    //  

    // Public attributes
    //  

    pcm_t data;
    // how many items (i.e. floats) are there in data?
    size_t count;
    // how many floats to skip when starting with playback
    size_t offset;
    string Filename;
    SongFormat Format;


    /**
     * opens the current file using the corresponding lib
     */
    virtual void open ();


    /**
     */
    virtual void close ();


    /**
     * @param  items ensure that the buffer holds at least "items" floats
     */
    virtual void fillBuffer (unsigned int items);


    /**
     */
    virtual void releaseBuffer ();


    /**
     * @return vector
     */
    virtual vector getLoops () const;


    /**
     * returns number of frames that are (expected to be) in pcm buffer
     * @return unsigned int
     */
    virtual unsigned int getFrames () const;

protected:

    // Static Protected attributes
    //  

    // Protected attributes
    //  

public:

protected:

public:

protected:


private:

    // Static Private attributes
    //  

    // Private attributes
    //  

public:

private:

public:

private:


    void initAttributes () ;

};

#endif // PCMHOLDER_H
