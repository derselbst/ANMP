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

#if 0
void LoudnessFile::write(ebur128_state* state, string filePath) noexcept
{
    filePath = toebur128Filename(filePath);

    FILE* f = fopen(filePath.c_str(), "wb");

    if(f==nullptr)
    {
        // TODO: log
        return;
    }

    double lufsLoudness;
    if(ebur128_true_peak(state, &lufsLoudness) == EBUR128_SUCCESS)
    {
        fwrite(&lufsLoudness, 1, sizeof(double), f);
    }
    else
    {
        fwrite(&TargetLUFS, 1, sizeof(double), f);
    }

    fclose(f);
}
#endif

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
