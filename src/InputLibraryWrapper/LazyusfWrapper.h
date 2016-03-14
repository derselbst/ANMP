#ifndef LAZYUSFWRAPPER_H
#define LAZYUSFWRAPPER_H

#include "Song.h"

/**
  * class LazyusfWrapper
  *
  */
class LazyusfWrapper : public Song
{
public:

    // Constructors/Destructors
    //

    LazyusfWrapper(string filename, size_t fileOffset=0, size_t fileLen=0);

    /**
     * Empty Destructor
     */
    virtual ~LazyusfWrapper();
    
    /**
     * opens the current file using the corresponding lib
     */
    void open () override;


    /**
     */
    void close () override;


    /** PCM buffer fill call to underlying library goes here
     */
    void fillBuffer () override;


    /**
     */
    void releaseBuffer () override;

    /**
     * returns number of frames this song lasts, they dont necessarily have to be in the pcm buffer at one time
     * @return unsigned int
     */
    frame_t getFrames () const override;

private:
  // set by usf_info
  unsigned int enable_compare = 0;
  // set by usf_info
  unsigned int enable_fifo_full = 0;
  
  // length in ms to fade
  unsigned long fade_ms = 0;
  
  unsigned char* usfHandle = nullptr;
  
public:  
 static void * stdio_fopen( const char * path, const char * mode );

 static size_t stdio_fread( void *p, size_t size, size_t count, void *f );

 static int stdio_fseek( void * f, int64_t offset, int whence );

 static int stdio_fclose( void * f );

 static long stdio_ftell( void * f );


static int usf_loader(void * context, const uint8_t * exe, size_t exe_size, const uint8_t * reserved, size_t reserved_size);


static int usf_info(void * context, const char * name, const char * value);

    
};

#endif // LAZYUSFWRAPPER_H
