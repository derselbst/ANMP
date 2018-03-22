#ifndef LIBGMEWRAPPER_H
#define LIBGMEWRAPPER_H

#include "StandardWrapper.h"

#include <future>
#include <gme.h>

/**
  * class LibGMEWrapper
  *
  * Wrapper for Game Music Emu (libgme), for supporting several video game music formats
  */
class LibGMEWrapper : public StandardWrapper<int16_t>
{
    public:
    // Constructors/Destructors
    //

    LibGMEWrapper(string filename);
    LibGMEWrapper(string filename, Nullable<size_t> fileOffset, Nullable<size_t> fileLen);
    void initAttr();

    // forbid copying
    LibGMEWrapper(LibGMEWrapper const &) = delete;
    LibGMEWrapper &operator=(LibGMEWrapper const &) = delete;

    virtual ~LibGMEWrapper();

    void open() override;

    void close() noexcept override;

    void fillBuffer() override;

    frame_t getFrames() const override;

    vector<loop_t> getLoopArray() const noexcept override;

    void render(pcm_t *bufferToFill, frame_t framesToRender = 0) override;

    void buildMetadata() noexcept override;

    private:
    Music_Emu *handle = nullptr;
    gme_info_t *info = nullptr;

    static void printWarning(Music_Emu *emu);

    bool wholeSong() const;
};

#endif // LIBGMEWRAPPER_H
