// anmp-dump: a tool for decoding every file supported by anmp to raw Wave

#include <iostream>
#include <string>

#include <anmp.hpp>

#include <experimental/filesystem>
#include <thread>
#include <chrono>
#include <getopt.h>

using namespace std;
using namespace std::experimental::filesystem;

void onSongChanged(void* pthis)
{
  Song* s = static_cast<Playlist*>(pthis)->current();
  
  CLOG(LogLevel_t::Info, "Now handling: " << s->Filename);
}

static const struct option long_options[] =
{
    {"unroll-loop",  no_argument, NULL, 'l'},
    {"output",       no_argument, NULL, 'o'},
    {"help",         no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

void usage(char* prog)
{
    cout << "usage: " << prog << " [OPTIONS] FILES|DIRS\n"
         << "\n" 
         << "options:\n"
         << "      -l      --unroll-loop     If there are loops in an audio file, unroll them according to the interal settings.\n"
         << "      -o      --output PATTERN  Specifies the naming pattern for file names.\n"
         << "      -h      --help            Print this.\n"
         << endl;
}

int main(int argc, char* argv[])
{
    gConfig.Load();
    gConfig.audioDriver = AudioDriver_t::Wave;
    gConfig.useLoopInfo = false;
    gConfig.RenderWholeSong = false;
    gConfig.useAudioNormalization = true;

    if(argc <= 1)
    {
        usage(argv[0]);
    }
        
    
    /* getopt_long stores the option index here. */
    int option_index = 0;

    int ch;
    while((ch = getopt_long(argc, argv, "o:lh", long_options, &option_index)) != -1)
    {
        switch (ch)
        {
        case 'o':
            StringFormatter::Singleton().SetFormat(string(optarg));
            break;

        case 'l':
            gConfig.useLoopInfo = true;
            break;

        case 'h':
            usage(argv[0]);
            return 0;
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            abort();
        }
    }
    
    
    constexpr int Threads = 4;
    Playlist plist[Threads];
    
    int curThread = 0;
    
    for(int i=optind; i<argc; i++)
    {
        if(is_directory(argv[i]))
        {
            for (directory_entry dirEntry : recursive_directory_iterator(argv[i]))
            {
                if(is_regular_file(dirEntry.status()))
                {
                    PlaylistFactory::addSong(plist[curThread], dirEntry.path());
                    curThread = (curThread+1) % Threads;
                }
            }
        }
        else if(is_regular_file(argv[i]))
        {
            PlaylistFactory::addSong(plist[curThread], absolute(argv[i]));
            curThread = (curThread+1) % Threads;
        }
    }

    
    std::vector<Player> players;
    players.reserve(Threads);

    for(int i=0;i<Threads; ++i)
    {
        // terminate when reaching end of playlist
        plist[i].add(nullptr);
        
        Player p(&plist[i]);
        players.push_back(std::move(p));
        
        players.back().onCurrentSongChanged += make_pair(&plist[i], &onSongChanged);
    }
    
    for(int i=0;i<Threads; ++i)
    {
        players[i].play();
    }
    
    for(int i=0;i<Threads; ++i)
    {
        while(players[i].getIsPlaying())
        {
            this_thread::sleep_for(chrono::seconds(1));
        }
    }

    return 0;
}
