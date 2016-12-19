#include "LoudnessFile.h"
#include "Common.h"

#include <cmath>
#include <cstdio>

mutex LoudnessFile::mtx;

string LoudnessFile::toebur128Filename(string filePath)
{
    string file = mybasename(filePath);
    string dir = mydirname(filePath);

    file = "." + file + ".ebur128";

    return dir + "/" + file;
}

static const double Target = 1.0f;

void LoudnessFile::write(string filePath, const float& gainCorrection) noexcept
{
    filePath = toebur128Filename(filePath);
    
    std::lock_guard<std::mutex> lock(LoudnessFile::mtx);
    
    FILE* f = fopen(filePath.c_str(), "wb");
    if(f==nullptr)
    {
        // TODO: log
        return;
    }

    fwrite(&gainCorrection, 1, sizeof(float), f);
    fclose(f);
}

/**
 * tries to find the corresponding loudnessfile for filePath and reads its loudness info
 *
 * @return gain correction factor (i.e. a relative gain), 1.0 is full amplitude (i.e. no correction necessary),
 */
float LoudnessFile::read(string filePath) noexcept
{
    filePath = toebur128Filename(filePath);

    std::lock_guard<std::mutex> lock(LoudnessFile::mtx);
    
    FILE* f = fopen(filePath.c_str(), "rb");
    
    float gain = Target;
    if(f!=nullptr)
    {
        fread(&gain, 1, sizeof(float), f);

        fclose(f);
    }

    return gain;
}
