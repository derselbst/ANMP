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

#ifdef USE_EBUR128
void LoudnessFile::write(ebur128_state* state, string filePath) noexcept
{
    double overallSamplePeak=0.0;
    for(unsigned int c = 0; c<state->channels; c++)
    {
      double peak = -0.0;
      if(ebur128_sample_peak(state, c, &peak) == EBUR128_SUCCESS)
      {
	overallSamplePeak = max(peak, overallSamplePeak);
      }
    }
    
    float gainCorrection = overallSamplePeak;
    if(gainCorrection<=0.0)
    {
        // TODO: log
    	return;
    }
    
    filePath = toebur128Filename(filePath);
    FILE* f = fopen(filePath.c_str(), "wb");
    if(f==nullptr)
    {
        // TODO: log
        return;
    }
    
    fwrite(&gainCorrection, 1, sizeof(float), f);
    fclose(f);
}
#endif

/**
 * tries to find the corresponding loudnessfile for filePath and reads its loudness info
 * 
 * @return gain correction factor (i.e. a relative gain), 1.0 is full amplitude (i.e. no correction necessary),
 */
float LoudnessFile::read(string filePath) noexcept
{    
    filePath = toebur128Filename(filePath);
    
    FILE* f = fopen(filePath.c_str(), "rb");
    
    float gain = Target;
    if(f!=nullptr)
    {
        fread(&gain, 1, sizeof(float), f);
        
        fclose(f);
    }
    
    return gain;
}
