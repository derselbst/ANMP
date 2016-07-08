#pragma once

#include <string>

#ifdef USE_EBUR128
#include <ebur128.h>
#endif

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

#ifdef USE_EBUR128
    static void write(ebur128_state* state, string filePath) noexcept;
#endif
    static float read(string filePath) noexcept;

private:
    static string toebur128Filename(string file);
};
