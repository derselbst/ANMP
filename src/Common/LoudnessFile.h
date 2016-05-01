#ifndef PLAYLISTFACTORY_H
#define PLAYLISTFACTORY_H

#include <string>
#include <ebur128.h>

using namespace std;

class Song;


/**
  * class LoudnessFile
  *
  */

class LoudnessFile
{
public:

    // no object
    LoudnessFile() = delete;
    // no copy
    LoudnessFile(const LoudnessFile&) = delete;
    // no assign
    LoudnessFile& operator=(const LoudnessFile&) = delete;


    static void write(ebur128_state* state, string filePath) noexcept;
    static float read(string filePath) noexcept;
    
private:
    static string toebur128Filename(string file);
};

#endif // PLAYLISTFACTORY_H
