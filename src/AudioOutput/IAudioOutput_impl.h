#ifndef IAUDIOOUTPUT_IMPL_H
#define IAUDIOOUTPUT_IMPL_H

#include <algorithm>
#include <cmath> // lround

template<typename T> void IAudioOutput::getAmplifiedBuffer(const T* inBuffer, T* outBuffer, unsigned long items)
{
    for(unsigned long i = 0; i<items; i++)
    {
        outBuffer[i] = inBuffer[i] * this->volume;
    }
}

template<typename TIN, typename TOUT>
void IAudioOutput::Mix(const TIN* in, TOUT* out, const frame_t frames/*, function<TOUT(long double)> converter*/)
{
    const unsigned int nVoices = this->currentFormat.Voices;
    
    uint16_t N;
    {
        Nullable<uint16_t> c = this->GetOutputChannels();
        if(!c.hasValue)
        {
            throw invalid_argument("IAudioOutput::Mix() may only be called with a previously specified number of output channels");
        }
        N = c.Value;
    }
    
    for(unsigned int f=0; f < frames; f++)
    {
        std::fill(this->mixdownBuf.begin(), this->mixdownBuf.end(), 0.0);
        std::fill(this->channelsMixed.begin(), this->channelsMixed.end(), 0);
        
        // walk through all available voices. if not muted, they contribute to the mixed output
        for(unsigned int v=0; v < nVoices; v++)
        {
            const uint16_t vchan = this->currentFormat.VoiceChannels[v];
            if(!this->currentFormat.VoiceIsMuted[v])
            {
                const uint16_t channelsToMix = max(vchan, N);
                for(unsigned int m=0; m < channelsToMix; m++)
                {
                    this->mixdownBuf[m % N] += in[m % vchan];
                    this->channelsMixed[m % N] += 1;
                }
            }
            
            // advance the pointer to point to next audio frame
            in += vchan;
        }
        
        // write the mixed buffer to out
        for(unsigned int i=0; i<N; i++)
        {
            TOUT& o = out[f*N + i];
            if(this->channelsMixed[i] != 0)
            {
                // average the mixed channels;
                long double item = this->mixdownBuf[i] / this->channelsMixed[i];
                
                // amplify volume
                item *= this->volume;
                
                if(std::is_floating_point<TOUT>())
                {
                    // normalize
                    if(!std::is_floating_point<TIN>())
                    {
                        item /= (numeric_limits<TIN>::max()+1.0);
                    }
                    
                    // clip
                    if(item > 1.0)
                    {
                        item = 1.0;
                    }
                    else if(item < -1.0)
                    {
                        item = -1.0;
                    }
                    
                    o = item;
                }
                else
                {
                    // clip
                    if(item > numeric_limits<TOUT>::max())
                    {
                        o = numeric_limits<TOUT>::max();
                    }
                    else if(item < numeric_limits<TOUT>::min())
                    {
                        o = numeric_limits<TOUT>::min();
                    }
                    else
                    {
                        o = lround(item);
                    }
                }
            }
            else
            {
                o = 0;
            }
        }
    }
}


#endif // IAUDIOOUTPUT_IMPL_H
