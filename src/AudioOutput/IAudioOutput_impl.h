#ifndef IAUDIOOUTPUT_IMPL_H
#define IAUDIOOUTPUT_IMPL_H

#include <cmath> // lround
#include <type_traits>
#include <limits>


template<typename TIN, typename TOUT>
void IAudioOutput::Mix(const frame_t frames, const TIN *restrict in, const SongFormat &inputFormat, TOUT *restrict out) noexcept
{
    const auto N = this->GetOutputChannels();
    const unsigned int nVoices = inputFormat.Voices;

    // allocate a temporary mixdown buffer where a single frame of all the voices get added to
    // we cant use "out" directly, depending on TOUT this might overflow and would prevent proper clipping a few lines later
    double temp[std::numeric_limits<decltype(IAudioOutput::outputChannels)>::max()];

    for (unsigned int f = 0; f < frames; f++)
    {
        // zero the current frame
        for (unsigned int i = 0; i < N; i++)
        {
            temp[i] = 0;
        }

        // walk through all available voices. if not muted, they contribute to the mixed output
        for (unsigned int v = 0; v < nVoices; v++)
        {
            const uint16_t vchan = inputFormat.VoiceChannels[v];
            if (vchan > 0 && !inputFormat.VoiceIsMuted[v])
            {
                const uint16_t channelsToMix = max<uint16_t>(vchan, N);
                for (unsigned int m = 0; m < channelsToMix; m++)
                {
                    temp[m % N] += in[m % vchan];
                }
            }

            // advance the pointer to point to next bunch of voice items
            in += vchan;
        }

        // write the mixed buffer to out
        for (unsigned int i = 0; i < N; i++)
        {
            TOUT &o = out[f * N + i];
            // average the mixed channels;
            auto item = temp[i];

            // amplify volume
            item *= this->volume;

            if (std::is_floating_point<TOUT>())
            {
                // normalize
                if (!std::is_floating_point<TIN>())
                {
                    item /= (numeric_limits<TIN>::max() + 1.0);
                }

                // clip
                o = min(max(item, -1.0), 1.0);
            }
            else
            {
                // clip
                constexpr auto MAX = numeric_limits<TOUT>::max();
                constexpr auto MIN = numeric_limits<TOUT>::min();
                if (item > MAX)
                {
                    o = MAX;
                }
                else if (item < MIN)
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
