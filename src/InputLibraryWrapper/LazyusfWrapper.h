#ifndef LAZYUSFWRAPPER_H
#define LAZYUSFWRAPPER_H

#include "StandardWrapper.h"

/**
  * class LazyusfWrapper
  *
  * Wrapper for lazyusf, for supporting the Ultra64 Sound Format, enabling playback of many beloved N64 titles
  */
class LazyusfWrapper : public StandardWrapper<int16_t>
{
public:

    // Constructors/Destructors
    //
    LazyusfWrapper(string filename);
    LazyusfWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

    // forbid copying
    LazyusfWrapper(LazyusfWrapper const&) = delete;
    LazyusfWrapper& operator=(LazyusfWrapper const&) = delete;

    virtual ~LazyusfWrapper();

    void open () override;

    void close () noexcept override;

    void fillBuffer () override;

    frame_t getFrames () const override;

    void buildMetadata() noexcept override;

    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;


private:
    // set by usf_info
    unsigned int enable_compare = 0;
    // set by usf_info
    unsigned int enable_fifo_full = 0;

    // length in ms to fade
    unsigned long fade_ms = 0;

    unsigned char* usfHandle = nullptr;

public:
    static void * stdio_fopen( const char * path );

    static size_t stdio_fread( void *p, size_t size, size_t count, void *f );

    static int stdio_fseek( void * f, int64_t offset, int whence );

    static int stdio_fclose( void * f );

    static long stdio_ftell( void * f );


    static int usf_loader(void * context, const uint8_t * exe, size_t exe_size, const uint8_t * reserved, size_t reserved_size);


    static int usf_info(void * context, const char * name, const char * value);


};

#endif // LAZYUSFWRAPPER_H
