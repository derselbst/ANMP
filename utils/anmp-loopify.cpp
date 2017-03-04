

#include <iostream>

#include <anmp.hpp>

#include <experimental/filesystem>
#include <cmath>

using namespace std;
using namespace std::experimental::filesystem;

/**
 * @brief checks for same amplitude and same slope at "left" and "right"
 * 
 * Checks whether the frames given by "left" and "right" can be used as looppoints for "song". Therefore we check that "delta" frames before left and right match the epsilon environment respectively.
 */
template<typename T>
bool evaluate(const Song* song, frame_t left, frame_t right, const frame_t delta, const long long epsilon)
{
    bool result = true;
    T* buffer = static_cast<T*>(song->data);
    left *= song->Format.Channels;
    right *= song->Format.Channels;

    // walk along the delta environment
    for (int i=0; result && i<delta*song->Format.Channels; i++)
    {
//         if(left-i < 0 || left+i>song->count || right+i > song->count)
//         {
//             cerr << "Dings: left " << left << " right " << right << endl;
//         }
        // does the amplitude left and right in the delta environment match the specified epsilon env?
//         result&= fabs(static_cast<float>(buffer[left-delta]) - static_cast<float>(buffer[right-delta])) <= epsilon;
        result&= fabs(static_cast<float>(buffer[left+i]) - static_cast<float>(buffer[right+i])) <= epsilon;
    }    
    
    return result;
}

void usage(char *progName)
{
    cout << "Usage: " << progName << " <File> [epsilon] [delta]\n\tdelta: amount of audio that must match before each potential loop start and stop given in milliseconds (i.e. diff within domain)\n\tepsilon: how much the amplitudes of loop start and stop may differ (i.e. diff within codomain, ideally diff is zero)" << endl;
}

int main(int argc, char** argv)
{
    gConfig.useLoopInfo = false;
    gConfig.RenderWholeSong = true;
    gConfig.useAudioNormalization = false;
    
    // make sure StandardWrapper::fillBuffer returns when buffer is fully filled
    gConfig.PreRenderTime = 0;
    
    frame_t delta = 40;
    long long epsilon = 5;

    string file;
    switch(argc)
    {
    case 4:
        delta = atoll(argv[3]);
        [[fallthrough]]
    case 3:
        epsilon = atoll(argv[2]);
        [[fallthrough]];
    case 2:
        file = argv[1];
        break;
    default:
        usage(argv[0]);
        return -1;
    }
    
    if(!is_regular_file(file))
    {
        cerr << "not a regular file." << endl;
        return -1;
    }
    
    Playlist plist;
    PlaylistFactory::addSong(plist, file);
    plist.add(nullptr);
    
    
    Song* song = plist.current();
    do
    {
        song->open();
        song->fillBuffer();
        
        for (frame_t left = delta; left < song->getFrames()-2*delta && left < 90000; left+=delta/2)
        {
            for (frame_t right = song->getFrames()-1-delta; left+delta < right && right > 6000000; right-=delta/2)
            {
                bool match = false;
                
                switch (song->Format.SampleFormat)
                {
                case SampleFormat_t::float32:
                {
                    match = evaluate<float>(song, left, right, delta, epsilon);
                    break;
                }
                case SampleFormat_t::int16:
                {
                    match = evaluate<int16_t>(song, left, right, delta, epsilon);
                    break;
                }
                case SampleFormat_t::int32:
                {
                    match = evaluate<int32_t>(song, left, right, delta, epsilon);
                    break;
                }
                case SampleFormat_t::unknown:
                    throw invalid_argument("pcmFormat mustnt be unknown");
                    break;

                default:
                    throw NotImplementedException();
                    break;
                }
                    
                if(match)
                {
                    cout << "Von " << left << " bis " << right << endl;
                }
            }
        }
        
        song->releaseBuffer();
        song->close();
    } while((song = plist.next()) != nullptr);

    return 0;
}