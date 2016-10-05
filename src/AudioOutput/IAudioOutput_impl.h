#ifndef IAUDIOOUTPUT_IMPL_H
#define IAUDIOOUTPUT_IMPL_H


template<typename T> void IAudioOutput::getAmplifiedBuffer(const T* inBuffer, T* outBuffer, unsigned long items)
{
    for(unsigned long i = 0; i<items; i++)
    {
        outBuffer[i] = inBuffer[i] * this->volume;
    }
}

#endif // IAUDIOOUTPUT_IMPL_H
