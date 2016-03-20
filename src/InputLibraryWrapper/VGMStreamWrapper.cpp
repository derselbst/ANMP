#include "VGMStreamWrapper.h"

#include "Common.h"
#include "Config.h"

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
  
}

void VGMStreamWrapper::open()
{
    handle = init_vgmstream(this->Filename.c_str());

    if (handle==nullptr)
    {
        throw runtime_error(string("Error: ") + __func__ + string(": failed opening \"") + this->Filename + "\"");
    }
    
    this->Format.Channels = handle->channels;
    this->Format.SampleRate = handle->sample_rate;
}

void VGMStreamWrapper::close()
{
    close_vgmstream (this->handle);
    this->handle=nullptr;
}

void VGMStreamWrapper::fillBuffer()
{
    if(this->data==nullptr)
    {
        this->count = this->getFrames() * this->Format.Channels;
        this->data = new int16_t[this->count];

        // usually this shouldnt block at all
        WAIT(this->futureFillBuffer);

        // immediatly start filling the pcm buffer
        this->futureFillBuffer = async(launch::async, &VGMStreamWrapper::asyncFillBuffer, this);

        // give libsnd at least a minor headstart
        this_thread::sleep_for (chrono::milliseconds(5));
    }
}

void VGMStreamWrapper::asyncFillBuffer()
{
    unsigned int itemsToRender = Config::FramesToRender*this->Format.Channels;

    int fullFillCount = this->count/itemsToRender;
    for(int i = 0; i<fullFillCount && !this->stopFillBuffer; i++)
    {
        render_vgmstream(static_cast<int16_t*>(this->data)+i*itemsToRender, Config::FramesToRender, this->handle);
	swap_samples_le(static_cast<int16_t*>(this->data),itemsToRender);
    }

    if(!this->stopFillBuffer)
    {
        unsigned int lastItemsToRender = this->count % itemsToRender;
        render_vgmstream(static_cast<int16_t*>(this->data)+fullFillCount*itemsToRender, lastItemsToRender/this->Format.Channels, this->handle);
	swap_samples_le(static_cast<int16_t*>(this->data),lastItemsToRender);
    }
}

void VGMStreamWrapper::releaseBuffer()
{
    this->stopFillBuffer=true;
    WAIT(this->futureFillBuffer);

    delete [] static_cast<int16_t*>(this->data);
    this->data=nullptr;
    this->count = 0;

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
    
if(handle->loop_flag==1) // does stream contain loop information?
{
    loop_t l;
    l.start = handle->loop_start_sample;
    l.stop = handle->loop_end_sample;
    //TODO let the user adjust loopcount
    l.count = 2;
    res.push_back(l);
}

//     if(res.empty())
//     {
//         SF_INSTRUMENT inst;
//         int ret = sf_command (this->sndfile, SFC_GET_INSTRUMENT, &inst, sizeof (inst)) ;
//         if(ret == SF_TRUE && inst.loop_count > 0)
//         {
// 
//             for (int i=0; i<inst.loop_count; i++)
//             {
//                 loop_t l;
//                 l.start = inst.loops[i].start;
//                 l.stop  = inst.loops[i].end;
// 
//                 // WARNING: AGAINST RIFF SPEC ahead!!!
//                 // quoting RIFFNEW.pdf: "dwEnd: Specifies the endpoint of the loop
//                 // in samples (this sample will also be played)."
//                 // however (nearly) every piece of software out there ignores that and
//                 // specifies the sample excluded from the loop
//                 // THUS: submit to peer pressure
//                 l.stop -= 1;
// 
//                 l.count = inst.loops[i].count;
//                 res.push_back(l);
//             }
//         }
//     }
    return res;
}

frame_t VGMStreamWrapper::getFrames() const
{
  return this->handle->num_samples;
}
