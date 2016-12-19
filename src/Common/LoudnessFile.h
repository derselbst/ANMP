#pragma once

#include <string>
#include <mutex>

using namespace std;


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

    static void write(string filePath, const float& gain) noexcept;

    static float read(string filePath) noexcept;
    
private:
    static mutex mtx;
    
    static string toebur128Filename(string file);
};
