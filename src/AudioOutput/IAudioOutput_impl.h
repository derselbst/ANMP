#ifndef IAUDIOOUTPUT_IMPL_H
#define IAUDIOOUTPUT_IMPL_H


template<typename T> void IAudioOutput::getAmplifiedBuffer(const T* inBuffer, T* outBuffer, unsigned long items)
{
    for(unsigned long i = 0; i<items; i++)
    {
        outBuffer[i] = inBuffer[i] * this->volume;
    }
}

template<typename TOUT>
using sample_converter_t = TOUT (*)(long double item);



template<std::int16_t N, typename TIN, typename TOUT=TIN>
int IAudioOutput::Mix(const TIN* in, TOUT* out, const frame_t frames, sample_converter_t<TIN, TOUT> convert = [](long double item){ return static_cast<TOUT>(item); })
{
    const unsigned int nVoices   = this->currentFormat.Voices;
    const unsigned int nChannels = this->currentFormat.Channels();
    
    // temporary double mixdown buffer, where all voices get added to
    std::array<long double, N> temp;
    // how often a voice channel has been added to temp
    std::array<uint16_t, N> channelsMixed;
    
    for(unsigned int f=0; f < frames; f++)
    {
        temp.fill(0.0);
        channelsMixed.fill(0);
        
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
                    channelsMixed[m % N] += 1;
                }
            }
            
            in += vchan;
        }
        
        // write the mixed temp buffer to out
        for(unsigned int i=0; i<N; i++)
        {
            TOUT& out = out[f*N + i];
            if(channelsMixed[i] != 0)
            {
                // average the mixed channels;
                long double item = temp[i] / channelsMixed[i];
                
                // amplify volume
                item *= this->volume;
                
                if(std::is_floating_point<TOUT>)
                {
                    // clip
                    if(item > 1.0)
                    {
                        item = 1.0;
                    }
                    else if(item < -1.0)
                    {
                        item = -1.0;
                    }
                    
//                     out = item;
                }
                else
                {
                    // clip
                    if(item > std::numeric_limits<TOUT>::max())
                    {
                        out = std::numeric_limits<TOUT>::max();
                    }
                    else if(item < std::numeric_limits<TOUT>::min())
                    {
                        out = std::numeric_limits<TOUT>::min();
                    }
                    else
                    {
//                         out = lround(item);
                    }
                }
                
                out = convert(item)
            }
            else
            {
                out = 0;
            }
        }
    }
}


#endif // IAUDIOOUTPUT_IMPL_H
