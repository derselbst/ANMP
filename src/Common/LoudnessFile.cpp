#include "LoudnessFile.h"
#include "Common.h"

#include <cmath>
#include <cstdio>

string LoudnessFile::toebur128Filename(string filePath)
{
    string file = mybasename(filePath);
    string dir = mydirname(filePath);
    
    file = "." + file + ".ebur128";
    
    return dir + "/" + file;
}

static const double TargetLUFS = -23.0f;

#ifdef USE_EBUR128
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
    if(ebur128_loudness_global(state, &lufsLoudness) == EBUR128_SUCCESS)
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

/**
 * tries to find the corresponding loudnessfile for filePath and reads its loudness info
 * 
 * @return gain factor, which can be simply multiplied with the pcm frames to adjust volume
 */
float LoudnessFile::read(string filePath) noexcept
{    
    filePath = toebur128Filename(filePath);
    
    FILE* f = fopen(filePath.c_str(), "rb");
    
    double lufsLoudness;
    if(f==nullptr)
    {
        lufsLoudness = TargetLUFS;
    }
    else
    {
        fread(&lufsLoudness, 1, sizeof(double), f);
        
        fclose(f);
    }
    double dezibel = TargetLUFS - lufsLoudness;
    
    float gain = pow(10, dezibel / 20);
    
    return gain;
}
