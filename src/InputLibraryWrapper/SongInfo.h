#ifndef SONGINFO_H
#define SONGINFO_H

#include <string>

/**
  * class SongInfo
  *
  */

// IMPLEMENTERS: when adding new members here dont forget to update PlaylistFactory (i.e. parsing metadata from cue sheets and handling of overridingMetadata)
struct SongInfo
{
    std::string Track = "";
    std::string Title = "";
    std::string Artist = "";
    std::string Album = "";
    std::string Composer = "";
    std::string Year = "";
    std::string Genre = "";
    std::string Comment = "";
};

#endif // SONGINFO_H
