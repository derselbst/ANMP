#include "LoudnessFile.h"
#include "Common.h"

#include <cmath>

string LoudnessFile::toebur128Filename(string filePath)
{
    string file = mybasename(filePath);
    string dir = mydirname(filePath);
    
    file = "." + file + ".ebur128";
    
    return dir + "/" + file;
}

void LoudnessFile::write(ebur128_state* state, string filePath) noexcept
{
    filePath = toebur128Filename(filePath);
    
    FILE* f = fopen(filePath.c_str(), "w");
    
    if(f==nullptr)
    {
        // TODO: log
        return;
    }
    
    double lufsLoudness;
    ebur128_loudness_global(state, &lufsLoudness);
    
    fprintf(f, "%.2f LUFS\n", lufsLoudness);
    
    fclose(f);
}

/**
 * tries to find the corresponding loudnessfile for filePath and reads its loudness info
 * 
 * @return gain factor, which can be simply multiplied with the pcm frames to adjust volume
 */
float LoudnessFile::read(string filePath) noexcept
{
    const float TargetLUFS = -23.0f;
    
    filePath = toebur128Filename(filePath);
    
    FILE* f = fopen(filePath.c_str(), "r");
    
    float lufsLoudness;
    if(f==nullptr)
    {
        lufsLoudness = TargetLUFS;
    }
    else
    {
        fscanf(f, "%.2f", &lufsLoudness);
        
        fclose(f);
    }
    float dezibel = TargetLUFS - lufsLoudness;
    
    float gain = pow(10, dezibel / 20);
    
    return gain;
}
