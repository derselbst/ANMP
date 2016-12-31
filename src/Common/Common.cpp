#include "Common.h"
#include "CommonExceptions.h"
#include "AtomicWrite.h"


#include <iostream>
#include <cctype>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>

// #include <filesystem>

#ifdef _POSIX_SOURCE
#include <strings.h> // strncasecmp
#include <sys/stat.h>
extern "C"
{
#include <libgen.h> // basename, dirname
}
#endif

using namespace std;
// namespace fs = std::filesystem;

#ifndef _POSIX_SOURCE
// helper function for case insensitive string compare
// will workon all platforms, but probably slow
bool iEqualsUgly(string strFirst, string strSecond)
{
    // convert strings to upper case before compare
    transform(strFirst.begin(), strFirst.end(), strFirst.begin(), [](unsigned char c)
    {
        return std::toupper(c);
    });
    transform(strSecond.begin(), strSecond.end(), strSecond.begin(), [](unsigned char c)
    {
        return std::toupper(c);
    });
    return strFirst == strSecond;
}
#endif

// helper function for case insensitive string compare
bool iEquals(const string& str1, const string& str2)
{
#ifdef _POSIX_SOURCE
    if(str1.size() != str2.size())
    {
        return false;
    }
    return strncasecmp(str1.c_str(), str2.c_str(), str1.size()) == 0;
#else
    return iEqualsUgly(str1, str2);
#endif
}

string getFileExtension(const string& filePath)
{
    return filePath.substr(filePath.find_last_of(".") + 1);
}

#define BORK_TIME 0xC0CAC01A
/** @brief converts a time string to ms
 *
 * @param[in] input a string in the format of mm:ss where mm=minutes and ss=seconds
 * @return an integer in milliseconds
 */
unsigned long parse_time_crap(const char *input)
{
    unsigned long value = 0;
    unsigned long multiplier = 1000;
    const char * ptr = input;
    unsigned long colon_count = 0;

    while (*ptr && ((*ptr >= '0' && *ptr <= '9') || *ptr == ':'))
    {
        colon_count += *ptr == ':';
        ++ptr;
    }
    if (colon_count > 2)
    {
        return BORK_TIME;
    }
    if (*ptr && *ptr != '.' && *ptr != ',')
    {
        return BORK_TIME;
    }
    if (*ptr)
    {
        ++ptr;
    }
    while (*ptr && *ptr >= '0' && *ptr <= '9')
    {
        ++ptr;
    }
    if (*ptr)
    {
        return BORK_TIME;
    }

    ptr = strrchr(input, ':');
    if (!ptr)
    {
        ptr = input;
    }
    for (;;)
    {
        char * end;
        if (ptr != input)
        {
            ++ptr;
        }
        if (multiplier == 1000)
        {
            double temp = strtod(ptr, &end);
            if (temp >= 60.0)
            {
                return BORK_TIME;
            }
            value = (long)(temp * 1000.0f);
        }
        else
        {
            unsigned long temp = strtoul(ptr, &end, 10);
            if (temp >= 60 && multiplier < 3600000)
            {
                return BORK_TIME;
            }
            value += temp * multiplier;
        }
        if (ptr == input)
        {
            break;
        }
        ptr -= 2;
        while (ptr > input && *ptr != ':')
        {
            --ptr;
        }
        multiplier *= 60;
    }

    return value;
}

string framesToTimeStr(frame_t frames, const unsigned int& sampleRate)
{
    int sec = frames/sampleRate;
    int min = sec / 60;
    sec %= 60;

    stringstream ssTime;
    ssTime << min << ":" << setw(2) << setfill('0') << sec;

    // stringstream::str() returns a temporary object
    string temp = ssTime.str();
    // return a deep copy of that string
    return string(temp.c_str());
}

/**
 * @param[in] ms position in audiofile in milliseconds
 * @param[in] sampleRate sample rate of audiofile in HZ
 *
 * @return position in audiofile in frames (i.e. samples)
 */
frame_t msToFrames(const size_t& ms, const unsigned int& sampleRate)
{
    return (ms*sampleRate)/1000;
}


#ifdef _WIN32
#include <stdlib.h>

char driveBuf[_MAX_DRIVE];
char dirBuf[_MAX_DIR];
char fnameBuf[_MAX_FNAME];
char extBuf[_MAX_EXT];
#endif

string mybasename(const string& path)
{
    string s = string(path.c_str());

#ifdef _POSIX_SOURCE
    return string(basename( const_cast<char*>(s.c_str()) ));
#elif _WINDOWS
    _splitpath(s.c_str(),
               nullptr, // drive
               dirBuf, // dir
               fnameBuf, // filename
               extBuf);

    if(fnameBuf[0] = '\0')
    {
        throw NotImplementedException();
    }
    else
    {
        string result = string(fnameBuf);
        result.append(extBuf);
        return result;
    }

#else
#error "Unsupported Platform"
#endif
}

string mydirname(const string& path)
{
    string s = string(path.c_str());
    return string(dirname( const_cast<char*>(s.c_str()) ));
}

string getUniqueFilename(const string& path)
{
    constexpr int Max = 1000;
    const string ext = getFileExtension(path);
    int i = 0;
    
    string unique = path;
    while(myExists(unique) && i<Max)
    {
        unique = string(path.c_str());
        
        // remove the extension
        unique.erase(unique.find_last_of(".")+1);
        
        // add a unique number with leading zeros
        unique += string(log10(Max) - to_string(i).length(), '0') + to_string(i);
        
        // readd the extension
        unique += "." + ext;
        
        i++;
    }
    
    if(i>=Max)
    {
        throw runtime_error("failed to get unique filename, max try count exceeded");
    }
    
     return unique;
}

size_t getFileSize(FILE* f)
{
    int fd = fileno(f);
    return getFileSize(fd);
}

size_t getFileSize(int fd)
{
    struct stat stat;
    if (fstat(fd, &stat) == -1)
    {
        THROW_RUNTIME_ERROR("fstat failed (" << strerror(errno) << ")");
    }

    return stat.st_size;
}

bool myExists(const string& name)
{
    if(name.empty())
    {
        return false;
    }
    
#ifdef _POSIX_SOURCE
    struct stat buffer;
    int ret = stat(name.c_str(), &buffer);
    return ret == 0;
#endif

    if (FILE *file = fopen(name.c_str(), "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
}

Nullable<string> findSoundfont(string midFile)
{
    // trim extension
    midFile = midFile.erase(midFile.find_last_of("."), string::npos);

    string soundffile = midFile + ".sf2";

//     if(fs::exists(midFile + ".sf2"))
    if(myExists(soundffile))
    {
        // a soundfont named like midi file, but with sf2 extension
        return Nullable<string>(soundffile);
    }

    // get path to directory this file is in
    string dir = mydirname(midFile);

    soundffile = dir + "/"; // the directory this soundfont might be in
    soundffile += mybasename(dir); // bare name of that soundfont
    soundffile += ".sf2"; // extension

//     if(fs::exists(dir + ".sf2"))
    if(myExists(soundffile))
    {
        return Nullable<string>(soundffile);
    }

//     for(directory_entry& dirEntry : fs::directory_iterator("sandbox"))
//     {
//         string ext = getFileExtension(dirEntry.path());
//
//         if(iEquals(ext, "sf2"))
//         {
//             // a soundfont in that directory
//             return Nullable<string>(dirEntry.path());
//         }
//     }

    return Nullable<string>(nullptr);
}
