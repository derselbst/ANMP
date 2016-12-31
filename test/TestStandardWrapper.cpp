
#include <iostream>
#include <string>

#include "StandardWrapper.h"
#include "Test.h"

using namespace std;


#define GEN_FRAMES(FORMAT, MAX, ITEM) ((float)(ITEM) / (MAX))

template<typename FORMAT>
class TestSong : public StandardWrapper<FORMAT>
{
public:
    TestSong(string filename) : StandardWrapper<FORMAT>(filename)
    {}

    // forbid copying
    TestSong(TestSong const&) = delete;
    TestSong& operator=(TestSong const&) = delete;

    ~TestSong() override
    {
        this->releaseBuffer();
        this->close();
    }

    void open () override {}

    void close () noexcept override {}

    void fillBuffer () override
    {
        StandardWrapper<FORMAT>::fillBuffer(this);
    }

    frame_t getFrames () const override
    {
        return 100;
    }
    
    void render(pcm_t* bufferToFill, frame_t framesToRender=0) override
    {
        STANDARDWRAPPER_RENDER( FORMAT,
                                for(unsigned int i=0; i<framesToDoNow * this->Format.Channels; i++)
                                {
                                    pcm[i] = GEN_FRAMES(FORMAT, framesToDoNow * this->Format.Channels, i);
                                })
    }
};


int main()
{
    // render everything into one buffer, but make sure, buffer is fully filled after fillBuffer() call
    Config::PreRenderTime = 0;
    Config::RenderWholeSong = true;
    Config::useAudioNormalization = true;
    
    TestSong<float> testFloat("");
    testFloat.Format.SampleFormat = SampleFormat_t::float32;
    testFloat.Format.SampleRate = 22050;
    
    for(int c=1; c<=6; c++)
    {
        testFloat.Format.Channels = c;

        testFloat.open();
        
        testFloat.fillBuffer();
        TEST_ASSERT(testFloat.data != nullptr);
        
        unsigned int nItems = c * testFloat.getFrames();
        TEST_ASSERT(testFloat.count == nItems);
        
        float* pcm = static_cast<float*>(testFloat.data);
        for(unsigned int i=0; i < nItems; i++)
        {
            float item = GEN_FRAMES(float, nItems, i);
            TEST_ASSERT(pcm[i] == item);
        }
        
        TEST_ASSERT(testFloat.getFramesRendered() == testFloat.getFrames());
        
        testFloat.releaseBuffer();
        TEST_ASSERT(testFloat.data == nullptr);
        TEST_ASSERT(testFloat.count == 0);
        testFloat.close();
    }
    
    
    return 0;


}
