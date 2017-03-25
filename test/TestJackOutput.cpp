
#include <iostream>
#include <string>
#include <limits>

#include "JackOutput.h"
#include "Test.h"

using namespace std;


class JackUnderTest : public JackOutput
{
    friend int main();
    
    
    int doResampling(const float* inBuf, const size_t Frames) override
    {
        lock_guard<recursive_mutex> lock(this->mtx);
        
        // no resampling for testing here, just copy        
        for(unsigned int i=0; i< this->currentFormat.Channels()*Frames; i++)
        {
            this->interleavedProcessedBuffer.buf[i] = inBuf[i];
        }
        
        return Frames;
    }
};


int main()
{
    bool failed=false;

    JackUnderTest jack;
    try
    {
        jack.open();
        
        SongFormat f;
        f.SampleRate = jack_get_sample_rate(jack.handle);
        f.SetVoices(1);
        f.VoiceChannels[0] = 1;
        f.SampleFormat = SampleFormat_t::int16;
        jack.init(f);
        
        
        // "start playback"
        
        constexpr int16_t samples[] = { 1, 1, 1, 1,
                                        0, 0, 0, 0};
        
        jack.write(samples, 4);
        
        decltype(jack.interleavedProcessedBuffer.buf) buf = jack.interleavedProcessedBuffer.buf;
        remove_pointer<decltype(buf)>::type maxval = numeric_limits<remove_reference<decltype(samples[0])>::type>::max();
        maxval += 1;
        
        TEST_ASSERT_EQ(buf[0], samples[0]/maxval);
        TEST_ASSERT_EQ(buf[1], samples[1]/maxval);
        TEST_ASSERT_EQ(buf[2], samples[2]/maxval);
        TEST_ASSERT_EQ(buf[3], samples[3]/maxval);
        
        
        // play a new song
        
        f.VoiceChannels[0] = 1;
        jack.init(f);
        buf = jack.interleavedProcessedBuffer.buf;
        jack.write(samples+4, 4);
        
        TEST_ASSERT_EQ(buf[4], samples[4]/maxval);
        TEST_ASSERT_EQ(buf[5], samples[5]/maxval);
        TEST_ASSERT_EQ(buf[6], samples[6]/maxval);
        TEST_ASSERT_EQ(buf[7], samples[7]/maxval);
        
        
        
        f.VoiceChannels[0] = 2;
        jack.init(f);
        buf = jack.interleavedProcessedBuffer.buf;
        jack.write(samples, 4);
        
        TEST_ASSERT_EQ(buf[0], samples[0]/maxval);
        TEST_ASSERT_EQ(buf[1], samples[1]/maxval);
        TEST_ASSERT_EQ(buf[2], samples[2]/maxval);
        TEST_ASSERT_EQ(buf[3], samples[3]/maxval);
        TEST_ASSERT_EQ(buf[4], samples[4]/maxval);
        TEST_ASSERT_EQ(buf[5], samples[5]/maxval);
        TEST_ASSERT_EQ(buf[6], samples[6]/maxval);
        TEST_ASSERT_EQ(buf[7], samples[7]/maxval);
        
        
        constexpr int16_t samples2[] = { 50, 50, 55, 55,
                                         60, 60, 65, 65};
        
        f.VoiceChannels[0] = 2;
        jack.init(f);
        buf = jack.interleavedProcessedBuffer.buf;
        jack.write(samples2, 4);
        
        TEST_ASSERT_EQ(buf[0], samples2[0]/maxval);
        TEST_ASSERT_EQ(buf[1], samples2[1]/maxval);
        TEST_ASSERT_EQ(buf[2], samples2[2]/maxval);
        TEST_ASSERT_EQ(buf[3], samples2[3]/maxval);
        TEST_ASSERT_EQ(buf[4], samples2[4]/maxval);
        TEST_ASSERT_EQ(buf[5], samples2[5]/maxval);
        TEST_ASSERT_EQ(buf[6], samples2[6]/maxval);
        TEST_ASSERT_EQ(buf[7], samples2[7]/maxval);
        
        
        // now play a song with many channels
        f.VoiceChannels[0] = 8;
        jack.init(f);
        buf = jack.interleavedProcessedBuffer.buf;
        for(unsigned int i=0; i<jack.jackBufSize; i++)
        {
            jack.write(samples2, 1);
            // manually advance the buffer, to fill it up correctly
            jack.interleavedProcessedBuffer.buf += 1*f.Channels();
        }
        jack.interleavedProcessedBuffer.buf = buf;
        
        for(unsigned int i=0; i<jack.jackBufSize; i++)
        {
            TEST_ASSERT_EQ(buf[0], samples2[0]/maxval);
            TEST_ASSERT_EQ(buf[1], samples2[1]/maxval);
            TEST_ASSERT_EQ(buf[2], samples2[2]/maxval);
            TEST_ASSERT_EQ(buf[3], samples2[3]/maxval);
            TEST_ASSERT_EQ(buf[4], samples2[4]/maxval);
            TEST_ASSERT_EQ(buf[5], samples2[5]/maxval);
            TEST_ASSERT_EQ(buf[6], samples2[6]/maxval);
            TEST_ASSERT_EQ(buf[7], samples2[7]/maxval);
            
            buf += 1*f.Channels();
        }
        
        // now replay the stereo song, to make sure buffer got cleared up, reallocated, etc.
        f.VoiceChannels[0] = 2;
        jack.init(f);
        buf = jack.interleavedProcessedBuffer.buf;
        jack.write(samples, 4);
        
        TEST_ASSERT_EQ(buf[0], samples[0]/maxval);
        TEST_ASSERT_EQ(buf[1], samples[1]/maxval);
        TEST_ASSERT_EQ(buf[2], samples[2]/maxval);
        TEST_ASSERT_EQ(buf[3], samples[3]/maxval);
        TEST_ASSERT_EQ(buf[4], samples[4]/maxval);
        TEST_ASSERT_EQ(buf[5], samples[5]/maxval);
        TEST_ASSERT_EQ(buf[6], samples[6]/maxval);
        TEST_ASSERT_EQ(buf[7], samples[7]/maxval);
        
    }
    catch(const AssertionException& e)
    {
      cerr << e.what() << endl;
      failed |= true;
    }
    
    jack.close();
    
    return failed ? -1 : 0;
}
