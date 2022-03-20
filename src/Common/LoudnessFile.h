#pragma once

#include <mutex>
#include <string>


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
    LoudnessFile(const LoudnessFile &) = delete;
    // no assign
    LoudnessFile &operator=(const LoudnessFile &) = delete;

    static void write(std::string filePath, const float &gainCorrection) noexcept;

    static float read(std::string filePath) noexcept;

    private:
    static std::mutex mtx;

    static std::string toebur128Filename(const std::string& filePath);
};
