
#pragma once

#include "StandardWrapper.h"

#include <aopsf.h>

/**
  * class AopsfWrapper
  *
  */
class AopsfWrapper : public StandardWrapper<int16_t>
{
    public:
    // Constructors/Destructors
    //
    AopsfWrapper(string filename);
    AopsfWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

    // forbid copying
    AopsfWrapper(AopsfWrapper const &) = delete;
    AopsfWrapper &operator=(AopsfWrapper const &) = delete;

    ~AopsfWrapper() override;

    void open() override;

    void close() noexcept override;

    void fillBuffer() override;

    frame_t getFrames() const override;

    void buildMetadata() noexcept override;

    void render(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender) override;


    private:
    int psfVersion = 0;

    // length in ms to fade
    unsigned long fade_ms = 0;

    PSX_STATE *psfHandle = nullptr;
    void *psf2fs = nullptr;

    // only for psf1 files: indicates whether psf_loader() is called the very first time
    bool first = true;

    public:
    static void *stdio_fopen(const char *path);

    static size_t stdio_fread(void *p, size_t size, size_t count, void *f);

    static int stdio_fseek(void *f, int64_t offset, int whence);

    static int stdio_fclose(void *f);

    static long stdio_ftell(void *f);

    static void console_log(void *context, const char *message);

    static int psf_loader(void *context, const uint8_t *exe, size_t exe_size, const uint8_t *reserved, size_t reserved_size);


    static int psf_info(void *context, const char *name, const char *value);
};
