#include "VGMStreamWrapper.h"

#include "Common.h"
#include "Config.h"
#include "AtomicWrite.h"

extern "C"
{
#include <util.h>
}
// Constructors/Destructors
//

VGMStreamWrapper::VGMStreamWrapper(string filename, size_t fileOffset, size_t fileLen) : Song(filename, fileOffset, fileLen)
{
    this->Format.SampleFormat = SampleFormat_t::int16;
}

VGMStreamWrapper::~VGMStreamWrapper ()
{
    this->releaseBuffer();
    this->close();
}

void VGMStreamWrapper::open()
{
    if(this->handle!=nullptr)
    {
        return;
    }

    this->handle = init_vgmstream(this->Filename.c_str());

    if (handle==nullptr)
    {
        throw runtime_error(string("Error: ") + __func__ + string(": failed opening \"") + this->Filename + "\"");
    }

    this->Format.Channels = this->handle->channels;
    this->Format.SampleRate = this->handle->sample_rate;
}

void VGMStreamWrapper::close()
{
    if(this->handle!=nullptr)
    {
        close_vgmstream (this->handle);
        this->handle=nullptr;
    }
}

void VGMStreamWrapper::fillBuffer()
{
    if(this->data==nullptr)
    {
        this->count = this->getFrames() * this->Format.Channels;
        this->data = new int16_t[this->count];
        CLOG(AtomicWrite::LogLevel::DEBUG, "vgmstream allocated buffer at 0x" << this->data << endl);

        // usually this shouldnt block at all
        WAIT(this->futureFillBuffer);

        // (pre-)render the first 1/3 second
        this->render(msToFrames(333, this->Format.SampleRate));

        // immediatly start filling the pcm buffer
        this->futureFillBuffer = async(launch::async, &VGMStreamWrapper::render, this, 0);

        // allow the render thread to do his work
        this_thread::yield();
    }
}

void VGMStreamWrapper::render(frame_t framesToRender)
{
    if(framesToRender==0)
    {
        // render rest of file
        framesToRender = this->getFrames()-this->framesAlreadyRendered;
    }
    else
    {
        framesToRender = min(framesToRender, this->getFrames()-this->framesAlreadyRendered);
    }

    int16_t* pcm = static_cast<int16_t*>(this->data);
    pcm += this->framesAlreadyRendered * this->Format.Channels;

    int framesToDoNow;
    while(framesToRender>0 && !this->stopFillBuffer)
    {
        framesToDoNow = (framesToRender/Config::FramesToRender)>0 ? Config::FramesToRender : framesToRender%Config::FramesToRender;

        render_vgmstream(pcm, framesToDoNow, this->handle);
        pcm += framesToDoNow * this->Format.Channels;
        this->framesAlreadyRendered += framesToDoNow;

        framesToRender -= framesToDoNow;
    }
}

void VGMStreamWrapper::releaseBuffer()
{
    this->stopFillBuffer=true;
    WAIT(this->futureFillBuffer);

    delete [] static_cast<int16_t*>(this->data);
    this->data=nullptr;
    this->count = 0;
    this->framesAlreadyRendered=0;

    this->stopFillBuffer=false;
}

vector<loop_t> VGMStreamWrapper::getLoopArray () const
{
    vector<loop_t> res;

    // TODO implement
//     /* force only if there aren't already loop points */
//     if (force_loop && !handle->loop_flag) {
//         /* this requires a bit more messing with the VGMSTREAM than I'm
//          * comfortable with... */
//         handle->loop_flag=1;
//         handle->loop_start_sample=0;
//         handle->loop_end_sample=handle->num_samples;
//         handle->loop_ch=calloc(handle->channels,sizeof(VGMSTREAMCHANNEL));
//     }
//
//     /* force even if there are loop points */
//     if (really_force_loop) {
//         if (!handle->loop_flag) handle->loop_ch=calloc(handle->channels,sizeof(VGMSTREAMCHANNEL));
//         handle->loop_flag=1;
//         handle->loop_start_sample=0;
//         handle->loop_end_sample=handle->num_samples;
//     }
//
//     if (ignore_loop) handle->loop_flag=0;

    if(this->handle!=nullptr && this->handle->loop_flag==1) // does stream contain loop information?
    {
        loop_t l;
        l.start = handle->loop_start_sample;
        l.stop = handle->loop_end_sample;
        //TODO let the user adjust loopcount
        l.count = 2;
        res.push_back(l);
    }

    return res;
}

frame_t VGMStreamWrapper::getFrames() const
{
    if(this->handle==nullptr)
    {
        return 0;
    }
    return this->handle->num_samples;
}
