
#include <iostream>
#include <string>

#include "StandardWrapper.h"
#include "Test.h"

using namespace std;


#define GEN_FRAMES(FORMAT, MAX, ITEM) ((float)(ITEM) / (MAX))

// minimal example implementation of StandardWrapper
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

template<typename T>
void TestMethod(TestSong<T>& songUnderTest)
{
    for(int c=1; c<=6; c++)
    {
        songUnderTest.Format.Channels = c;

        songUnderTest.open();
        
        songUnderTest.fillBuffer();
        TEST_ASSERT(songUnderTest.data != nullptr);
        
        unsigned int nItems = c * songUnderTest.getFrames();
        TEST_ASSERT_EQ(songUnderTest.count, nItems);
        
        T* pcm = static_cast<T*>(songUnderTest.data);
        for(unsigned int i=0; i < nItems; i++)
        {
            T item = static_cast<T>(GEN_FRAMES(T, nItems, i));
            TEST_ASSERT_EQ(pcm[i], item);
        }
        
        TEST_ASSERT(songUnderTest.getFramesRendered() == songUnderTest.getFrames());
        
        songUnderTest.releaseBuffer();
        TEST_ASSERT(songUnderTest.data == nullptr);
        TEST_ASSERT(songUnderTest.count == 0);
        songUnderTest.close();
    }
}


int main()
{
    // render everything into one buffer, but make sure, buffer is fully filled after fillBuffer() call
    Config::PreRenderTime = 0;
    Config::RenderWholeSong = true;
    Config::useAudioNormalization = true;
    
    TestSong<float> testFloat("");
    testFloat.Format.SampleFormat = SampleFormat_t::float32;
    testFloat.Format.SampleRate = 22050;
    TestMethod<float>(testFloat);
    
    TestSong<int32_t> testint32("");
    testint32.Format.SampleFormat = SampleFormat_t::int32;
    testint32.Format.SampleRate = 1;
    TestMethod<int32_t>(testint32);
    
    
    return 0;


}
