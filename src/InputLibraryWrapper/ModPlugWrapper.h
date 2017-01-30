#pragma once

#include "StandardWrapper.h"

#include <libmodplug/modplug.h>


/**
  * class ModPlugWrapper
  *
  */
class ModPlugWrapper : public StandardWrapper<int32_t>
{
public:
    ModPlugWrapper(string filename);
    ModPlugWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

    // forbid copying
    ModPlugWrapper(ModPlugWrapper const&) = delete;
    ModPlugWrapper& operator=(ModPlugWrapper const&) = delete;

    virtual ~ModPlugWrapper ();

    // interface methods declaration

    void open () override;

    void close () noexcept override;

    void fillBuffer () override;

    frame_t getFrames () const override;

    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override;

private:
    static ModPlug_Settings settings;
    
    FILE* infile = nullptr;
    unsigned char * infilebuf = nullptr;
    int infilelen = 0;
    ModPlugFile* handle = nullptr;
};

