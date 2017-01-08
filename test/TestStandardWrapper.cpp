
#include <iostream>
#include <string>

#include "StandardWrapper.h"
#include "Test.h"

using namespace std;


#define GEN_FRAMES(FORMAT, MAX, ITEM) static_cast<FORMAT>(((ITEM) * 1.0L) / (MAX))

// minimal example implementation of StandardWrapper
template<typename FORMAT>
class TestSong : public StandardWrapper<FORMAT>
{
    const frame_t frames;
public:
    TestSong(frame_t frames, string filename="") : StandardWrapper<FORMAT>(filename),frames(frames)
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
        return this->frames;
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
            T item = GEN_FRAMES(T, nItems, i);
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
    gConfig.PreRenderTime = 0;
    gConfig.RenderWholeSong = true;
    gConfig.useAudioNormalization = true;

    bool failed=false;

    try
    {
      TestSong<double> test(100);
      test.Format.SampleFormat = SampleFormat_t::float64;
      test.Format.SampleRate = 47999;
      TestMethod<double>(test);
    }
    catch(const AssertionException& e)
    {
      cerr << "testing double failed" << endl;
      cerr << e.what() << endl;
      failed |= true;
    }
    
    try
    {
      TestSong<float> testFloat(100);
      testFloat.Format.SampleFormat = SampleFormat_t::float32;
      testFloat.Format.SampleRate = 22050;
      TestMethod<float>(testFloat);
    }
    catch(const AssertionException& e)
    {
      cerr << "testing float failed" << endl;
      cerr << e.what() << endl;
      failed |= true;
    }
    
    try
    {
      TestSong<int32_t> test(1024);
      test.Format.SampleFormat = SampleFormat_t::int32;
      test.Format.SampleRate = 1;
      TestMethod<int32_t>(test);
    }
    catch(const AssertionException& e)
    {
      cerr << "testing int32 failed" << endl;
      cerr << e.what() << endl;
      failed |= true;
    }
    
    try
    {
      TestSong<int16_t> testint16(1024);
      testint16.Format.SampleFormat = SampleFormat_t::int16;
      testint16.Format.SampleRate = 11025;
      TestMethod<int16_t>(testint16);
    }
    catch(const AssertionException& e)
    {
      cerr << "testing int16 failed" << endl;
      cerr << e.what() << endl;
      failed |= true;
    }
    
    try
    {
      TestSong<uint8_t> testuint8(9876);
      testuint8.Format.SampleFormat = SampleFormat_t::uint8;
      testuint8.Format.SampleRate = 1234;
      TestMethod<uint8_t>(testuint8);
    }
    catch(const AssertionException& e)
    {
      cerr << "testing uint8 failed" << endl;
      cerr << e.what() << endl;
      failed |= true;
    }
    
    return failed ? -1 : 0;
}
