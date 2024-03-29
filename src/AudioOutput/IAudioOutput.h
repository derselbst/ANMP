#pragma once

#include "SongFormat.h"
#include "types.h"

#include <cstdint>
#include <vector>


/**
  * Abstract base class for all classes that handle audio playback in ANMP
  *
  * need to support a new Audio Playback API? --> derive this class and implement all abstract methods
  * 
  * basic data flow of PCM:
  *  - this->write() public method called with Song::data
  *  - public write() gives that pointer to private write() methods
  *  - private write() methods are specialized by child classes
  *    - there they usually call this->Mix() on the pcm buffer
  *    - Song::data's PCM gets mixed into custom allocated buffers within child classes
  *    - (for jack, this buffer will get (partly) resampled)
  *    - finally it will be played
  */
class IAudioOutput
{
    protected:
    // this is an interface, even if there were no pure virtual methods, allow
    // construction for child classes only
    IAudioOutput();

    public:
    // no copy or assignment
    IAudioOutput(IAudioOutput const &) = delete;
    IAudioOutput &operator=(IAudioOutput const &) = delete;

    // virtual destructor for proper cleanup
    virtual ~IAudioOutput();


    /**
     * opens a sound device, e.g. set buffer size, periods,... i.e. things that dont change while running ANMP
     *
     * can and shall be called initially (i.e. after object creation). can only be called again after a call to close()
     */
    virtual void open() = 0;

    /**
     * initializes a sound device, e.g. set samplerate, channels, i.e. settings that can change while running ANMP
     *
     * called every time a new song shall be played
     * 
     * a succeding call shall leave the PCM stream in a state in which it is possible to call this->write(). else an exception shall be thrown.
     */
    virtual void init(SongFormat &format, bool realtime = false) = 0;

    /**
     * Starts the PCM stream. Called only when "Play" button is pressed (i.e. playback shall start)
     *
     * If the stream is already started, no error shall be risen.
     */
    virtual void start() = 0;

    /**
     * Stops the PCM stream immediately, i.e. when returning from this->stop() all pcm must have been processed, favourably by dropping them at once, leaving no thread residing within this->write()
     * 
     * Usually called when requesting to stop playback.
     *
     * If the stream is already stopped, no error shall be risen.
     */
    virtual void stop() = 0;

    /**
     * closes the device, frees all ressources allocated by this->open()
     *
     * Multiple calls to this->close() without corresponding calls to this->open() shall raise no error.
     */
    virtual void close() = 0;

    /**
     * sets the playback volume
     * @param vol volume usually ranged [0.0,1.0]
     */
    void setVolume(float vol);

    // gets and sets the number of mixdown channels, all non muted voices of a song get mixed to
    uint8_t GetOutputChannels();
    // only call this when playback is paused, i.e. no call to this->write() is pending
    virtual void SetOutputChannels(uint8_t);

    void SetVoiceConfig(decltype(SongFormat::Voices) voices, decltype(SongFormat::VoiceChannels) &voiceChannels);
    void SetMuteMask(decltype(SongFormat::VoiceIsMuted) &mask);

    /**
     * pushes the pcm pointed to by frameBuffer to the underlying audio driver (or at least schedules it for pushing/playing)
     *
     * this generic version only takes care of pointer arithmetic, passing the call on to the specific write() methods below
     * 
     * if necessary, make a deep copy of frameBuffer, since it cannot be guaranteed that it is still alive after returning
     *
     * @param  frameBuffer buffer that holds the pcm
     * @param  frames no. of frames to be played from the buffer
     * @param  offset item offset within frameBuffer, i.e. how many items (determined
     * by pcmFormat) to skip; this parameter shall avoid the caller to take care of
     * pointer arithmetic
     *
     * @note "frameBuffer" has to contain at least ("frames"*this->currentChannelCount) items following "offset",
     * i.e. sizeof(frameBuffer)==(frames*this->currentChannelCount*sizeof(DataTypePointedToByBuffer))
     *
     * @return see private write()
     */
    int write(const pcm_t *frameBuffer, frame_t frames, size_t offset);


    protected:
    SongFormat currentFormat;

    // mixed_and_converted_to_float_but_not_resampled buffer used in this->write()
    std::vector<unsigned char> processedBuffer;

    // the current volume [0,1.0] to use, i.e. a factor by that the PCM gets amplified.
    float volume = 1.0f;

    /**
     * mixes all the available audio channels of pcm provided by @p in into the @p out pcm buffer
     * 
     * @param frames number of frames to mix from @p in to @p out. Thus also determines sizes of these pcm buffers.
     * @param in interleaved input pcm buffer, usually Song::data.
     * @param inputFormat Determines the number of audio channels and how they are grouped to voices for @p in.
     * @param out mixed, amplified and interleaved output pcm buffer.
     */
    template<typename TIN, typename TOUT = TIN>
    void Mix(const frame_t frames, const TIN *RESTRICT in, const SongFormat &inputFormat, TOUT *RESTRICT out) noexcept;

    /**
     * pushes the pcm pointed to by buffer to the underlying audio driver and by that causes it to play
     * 
     * we should use a template method here, but templates cannot be virtual, thus use overloads instead
     *
     * @param  buffer buffer that holds the pcm
     * @param  frames no. of frames to be played from the buffer
     *
     * @note buffer has to contain (frames*this->currentChannelCount) items, i.e. sizeof(*buffer)==(frames*this->currentChannelCount*sizeof(DataTypePointedToByBuffer))
     *
     * @return number of frames successfully pushed to underlying audio driver. if this is zero, nothing was played and the caller may try again, after waiting 1ms or so.
     *
     * @warning you can return a number smaller "frames" (but greater 0), however this case can only berecovered, if the whole song is hold in memory. you should better return 0 and play nothing, if you face such a problem.
     */
    virtual int write(const float *buffer, frame_t frames) = 0;
    virtual int write(const int16_t *buffer, frame_t frames) = 0;
    virtual int write(const int32_t *buffer, frame_t frames) = 0;
    
private:
    // number of audio channels all the different song's voices will be mixed to (by this->Mix())
    // if it has no value, no mixing takes place
    uint8_t outputChannels;
};
