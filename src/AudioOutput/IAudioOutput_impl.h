#ifndef IAUDIOOUTPUT_IMPL_H
#define IAUDIOOUTPUT_IMPL_H

#include <algorithm>
#include <cmath> // lround


template<typename TIN, typename TOUT>
void IAudioOutput::Mix(const TIN *restrict in, TOUT *restrict out, const frame_t frames)
{
    const unsigned int nVoices = this->currentFormat.Voices;
    
    // IAudioOutput::Mix() may only be called with a previously specified number of output channels
    // just silently assume that this happened
    const uint16_t N = this->GetOutputChannels().Value;
    
    // temporary mixdown buffer where all the voices get added to
    // we cant use "out" directly, this might overflow and would prevent proper clipping a few lines later
    vector<long double> mixdownBuf;
    // HACK to allocate space without zeroing it out
    mixdownBuf.reserve(N);
    // and then access the data via the pointer, not via vector::operator[], else index out of bounds assertion for MSVC
    long double* temp = mixdownBuf.data();
    
    for(unsigned int f=0; f < frames; f++)
    {
        // zero the current frame
        for(unsigned int i=0; i<N; i++)
        {
            temp[i] = 0;
        }
        
        // walk through all available voices. if not muted, they contribute to the mixed output
        for(unsigned int v=0; v < nVoices; v++)
        {
            const uint16_t vchan = this->currentFormat.VoiceChannels[v];
            if(!this->currentFormat.VoiceIsMuted[v])
            {
                const uint16_t channelsToMix = max(vchan, N);
                for(unsigned int m=0; m < channelsToMix; m++)
                {
                    temp[m % N] += in[m % vchan];
                }
            }
            
            // advance the pointer to point to next bunch of voice items
            in += vchan;
        }
        
        // write the mixed buffer to out
        for(unsigned int i=0; i<N; i++)
        {
            TOUT& o = out[f*N + i];
            // average the mixed channels;
            long double item = temp[i];
            
            // amplify volume
            item *= this->volume;
            
            if(std::is_floating_point<TOUT>())
            {
                // normalize
                if(!std::is_floating_point<TIN>())
                {
                    item /= (numeric_limits<TIN>::max()+1.0L);
                }
                
                // clip
                o = min(max(item, -1.0L), 1.0L);
            }
            else
            {
                // clip
                constexpr auto MAX = numeric_limits<TOUT>::max();
                constexpr auto MIN = numeric_limits<TOUT>::min();
                if(item > MAX)
                {
                    o = MAX;
                }
                else if(item < MIN)
                {
                    o = MIN;
                }
                else
                {
                    o = lround(item);
                }
            }
        }
    }
}


#endif // IAUDIOOUTPUT_IMPL_H
