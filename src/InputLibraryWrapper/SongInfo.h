#ifndef SONGINFO_H
#define SONGINFO_H

#include <string>

using namespace std;

/**
  * class SongInfo
  *
  */

// IMPLEMENTERS: when adding new members here dont forget to update PlaylistFactory (i.e. parsing metadata from cue sheets and handling of overridingMetadata)
struct SongInfo
{
    string Track = "";
    string Title = "";
    string Artist = "";
    string Album = "";
    string Composer = "";
    string Year = "";
    string Genre = "";
    string Comment = "";
};

#endif // SONGINFO_H
