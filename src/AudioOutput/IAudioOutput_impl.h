#ifndef IAUDIOOUTPUT_IMPL_H
#define IAUDIOOUTPUT_IMPL_H

#include <algorithm>
#include <cmath> // lround


template<typename TIN, typename TOUT>
void IAudioOutput::Mix(const TIN *restrict in, TOUT *restrict out, const frame_t frames/*, function<TOUT(long double)> converter*/)
{
    const unsigned int nVoices = this->currentFormat.Voices;
    
    // IAudioOutput::Mix() may only be called with a previously specified number of output channels
    // just silently assume that this happened
    const uint16_t N = this->GetOutputChannels().Value;
    
    for(unsigned int f=0; f < frames; f++)
    {
        for(unsigned int i=0; i<N; i++)
        {
            // zero the current frame
            out[f*N + i] = 0;
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
                    out[f*N + (m % N)] += in[m % vchan];
                }
            }
            
            // advance the pointer to point to next audio frame
            in += vchan;
        }
        
        // write the mixed buffer to out
        for(unsigned int i=0; i<N; i++)
        {
            TOUT& o = out[f*N + i];
            // average the mixed channels;
            long double item = o;
            
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
    }
}


#endif // IAUDIOOUTPUT_IMPL_H
