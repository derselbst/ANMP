
#pragma once

#include "AtomicWrite.h"
#include "CommonExceptions.h"
#include "Song.h"
#include <iostream>

template<typename T>
void PlaylistFactory::tryWith(Song *(&pcm), const string &filePath, Nullable<size_t> offset, Nullable<size_t> len)
{
    if (pcm == nullptr)
    {
        pcm = new T(filePath, offset, len);
        try
        {
            pcm->open();

            if (pcm->getFrames() <= 0)
            {
                THROW_RUNTIME_ERROR("Nothing to play, refusing to add file: '" << filePath << "'");
            }
        }
        catch (exception &e)
        {
            CLOG(LogLevel_t::Error, e.what());
            pcm->close();
            delete pcm;
            pcm = nullptr;
        }
    }
}