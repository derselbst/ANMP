
#pragma once

#include "Song.h"
#include <iostream>

template<typename T>
void PlaylistFactory::tryWith(Song* (&pcm), const string& filePath, Nullable<size_t> offset, Nullable<size_t> len)
{
    if(pcm==nullptr)
    {
        pcm = new T(filePath, offset, len);
        try
        {
            pcm->open();            
        }
        catch(exception& e)
        {
            cerr << e.what() << endl;
            pcm->close();
            delete pcm;
            pcm=nullptr;
        }
    }
}