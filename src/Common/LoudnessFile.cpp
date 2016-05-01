#include "LoudnessFile.h"
#include "Common.h"

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

float LoudnessFile::read(string filePath) noexcept
{
    filePath = toebur128Filename(filePath);
    
    FILE* f = fopen(filePath.c_str(), "r");
    
    if(f==nullptr)
    {
        // TODO: log
        return -23.0f;
    }
    
    float lufsLoudness;
    fscanf(f, "%.2f", &lufsLoudness);
    
    fclose(f);
}
