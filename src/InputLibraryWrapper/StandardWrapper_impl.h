#ifndef STANDARDWRAPPER_IMPL_H
#define STANDARDWRAPPER_IMPL_H

#include "Config.h"

template<typename IGNORE>
template<typename SAMPLEFORMAT>
void StandardWrapper<IGNORE>::doAudioNormalization(SAMPLEFORMAT *pcm, const frame_t framesToProcess)
{
    if (!gConfig.useAudioNormalization)
    {
        return;
    }

    const uint32_t Channels = this->Format.Channels();
    const unsigned int itemsToProcess = framesToProcess * Channels;

    /* audio normalization factor */
    const float AbsoluteGain = (numeric_limits<SAMPLEFORMAT>::max()) / (numeric_limits<SAMPLEFORMAT>::max() * this->gainCorrection);
    for (unsigned int i = 0; i < itemsToProcess && !this->stopFillBuffer; i++)
    {
        pcm[i] = static_cast<SAMPLEFORMAT>(pcm[i] * AbsoluteGain);
    }
}

#endif
