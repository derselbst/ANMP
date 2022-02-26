#include "Common.h"
#include "AtomicWrite.h"
#include "CommonExceptions.h"


#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

#ifdef _POSIX_SOURCE
#include <strings.h> // strncasecmp
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

extern "C" {
#include <libgen.h> // basename, dirname
}

#include <pwd.h>
#include <unistd.h>
#endif

// namespace fs = std::filesystem;

#ifndef _POSIX_SOURCE
// helper function for case insensitive std::string compare
// will workon all platforms, but probably slow
bool iEqualsUgly(std::string strFirst, std::string strSecond)
{
    // convert strings to upper case before compare
    transform(strFirst.begin(), strFirst.end(), strFirst.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    transform(strSecond.begin(), strSecond.end(), strSecond.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    return strFirst == strSecond;
}
#endif

// helper function for case insensitive std::string compare
bool iEquals(const std::string &str1, const std::string &str2)
{
#ifdef _POSIX_SOURCE
    if (str1.size() != str2.size())
    {
        return false;
    }
    return strncasecmp(str1.c_str(), str2.c_str(), str1.size()) == 0;
#else
    return iEqualsUgly(str1, str2);
#endif
}

std::string getFileExtension(const std::string &filePath)
{
    return filePath.substr(filePath.find_last_of('.') + 1);
}

/** @brief converts a time std::string to ms
 *
 * @param[in] input a std::string in the format of mm:ss where mm=minutes and ss=seconds
 * @return an integer in milliseconds
 */
unsigned long parse_time_crap(const char *input)
{
    unsigned long value = 0;
    unsigned long multiplier = 1000;
    const char *ptr = input;
    unsigned long colon_count = 0;

    while (*ptr && ((*ptr >= '0' && *ptr <= '9') || *ptr == ':'))
    {
        colon_count += *ptr == ':';
        ++ptr;
    }
    if (colon_count > 2)
    {
        THROW_RUNTIME_ERROR("too many colons in time std::string");
    }
    if (*ptr && *ptr != '.' && *ptr != ',')
    {
        THROW_RUNTIME_ERROR("unexpected character");
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
        THROW_RUNTIME_ERROR("time contains non-numeric character");
    }

    ptr = strrchr(input, ':');
    if (!ptr)
    {
        ptr = input;
    }
    for (;;)
    {
        char *end;
        if (ptr != input)
        {
            ++ptr;
        }
        if (multiplier == 1000)
        {
            double temp = strtod(ptr, &end);
            if (temp >= 60.0)
            {
                THROW_RUNTIME_ERROR("seconds and minutes must not be >= 60");
            }
            value = (long)(temp * 1000.0f);
        }
        else
        {
            unsigned long temp = strtoul(ptr, &end, 10);
            if (temp >= 60 && multiplier < 3600000)
            {
                THROW_RUNTIME_ERROR("seconds and minutes must not be >= 60");
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

std::string framesToTimeStr(frame_t frames, const unsigned int &sampleRate)
{
    int sec = frames / sampleRate;
    int min = sec / 60;
    sec %= 60;
    int msec100 = frames % sampleRate % 10;

    std::stringstream ssTime;
    ssTime << min << ":" << std::setw(2) << std::setfill('0') << sec << "." << msec100;

    // stringstream::str() returns a temporary object
    std::string temp = ssTime.str();
    // return a deep copy of that std::string
    return std::string(temp.c_str());
}

/**
 * @param[in] ms position in audiofile in milliseconds
 * @param[in] sampleRate sample rate of audiofile in HZ
 *
 * @return position in audiofile in frames (i.e. samples)
 */
frame_t msToFrames(const size_t &ms, const unsigned int &sampleRate)
{
    return (ms * sampleRate) / 1000;
}

/**
 * @param[in] ms position in audiofile in milliseconds
 * @param[in] sampleRate sample rate of audiofile in HZ
 *
 * @return position in audiofile in frames (i.e. samples)
 */
size_t framesToMs(const frame_t &frames, const unsigned int &sampleRate)
{
    return static_cast<size_t>((frames * 1000.0 / sampleRate) + .5);
}

#ifdef _WIN32
#include <stdlib.h>
#endif

std::string removeTrailingSlash(std::string s)
{
    size_t i;
    for (i = s.size(); i > 0; i--)
    {
        auto &c = s[i - 1];
        if (c == '/' || c == '\\')
        {
            continue;
        }
        else
        {
            break;
        }
    }
    s.resize(i);
    return s;
}

std::string mybasename(const std::string &path)
{

#if defined(_POSIX_SOURCE)
    std::string s = std::string(path.c_str());
    return std::string(basename(const_cast<char *>(s.c_str())));
#elif defined(_WIN32)
    std::string s = removeTrailingSlash(path);
    if (s == ".")
    {
        return ".";
    }
    char driveBuf[_MAX_DRIVE];
    char dirBuf[_MAX_DIR];
    char fnameBuf[_MAX_FNAME];
    char extBuf[_MAX_EXT];
    _splitpath(s.c_str(),
               nullptr, // drive
               dirBuf, // dir
               fnameBuf, // filename
               extBuf);

    if (fnameBuf[0] == '\0')
    {
        throw NotImplementedException();
    }
    else
    {
        std::string result = std::string(fnameBuf);
        result.append(extBuf);
        return result;
    }

#else
#error "Unsupported Platform"
#endif
}

std::string mydirname(const std::string &path)
{
    std::string s = removeTrailingSlash(path);
    std::filesystem::path p(s);
    std::string dir(p.parent_path().string());
    if (dir.empty())
    {
        return std::string(".");
    }
    return dir;
}

std::string getUniqueFilename(const std::string &path)
{
    constexpr int Max = 1000;
    const std::string ext = getFileExtension(path);
    int i = 0;

    std::string unique = path;
    while (myExists(unique) && i < Max)
    {
        unique = std::string(path.c_str());

        // remove the extension
        unique.erase(unique.find_last_of('.') + 1);

        // add a unique number with leading zeros
        unique += std::string(log10(Max) - std::to_string(i).length(), '0') + std::to_string(i);

        // readd the extension
        unique += "." + ext;

        i++;
    }

    if (i >= Max)
    {
        throw std::runtime_error("failed to get unique filename, max try count exceeded");
    }

    return unique;
}

size_t getFileSize(FILE *f)
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

bool myExists(const std::string &name)
{
    if (name.empty())
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

std::string myHomeDir()
{
    std::string home;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

    char *drive = getenv("HOMEDRIVE");
    char *path = getenv("HOMEPATH");

    if (drive == nullptr || *drive == '\0' || path == nullptr || *path == '\0')
    {
        THROW_RUNTIME_ERROR("failed to get home directory")
    }

    home = std::string(drive) + std::string(path);

#elif defined(_POSIX_SOURCE)

    const char *path = getenv("HOME");

    if (path == nullptr || *path == '\0')
    {
        struct passwd *pw = getpwuid(getuid());
        path = pw->pw_dir;

        if (path == nullptr || *path == '\0')
        {
            THROW_RUNTIME_ERROR("failed to get home directory")
        }
    }

    home = std::string(path);

#else
#error "Dont know how to determine the home directory on your platform"
#endif

    return home;
}

Nullable<std::string> findSoundfont(std::string midFile)
{
    static const char *ext[] = {".sf2", ".dls"};
    
    // trim extension
    midFile = midFile.erase(midFile.find_last_of('.'), std::string::npos);

    for(const char* e : ext)
    {
        std::string soundffile = midFile + e;

        //     if(fs::exists(midFile + e))
        if (myExists(soundffile))
        {
            // a soundfont named like midi file, but with sf2 extension
            return Nullable<std::string>(soundffile);
        }

        // get path to directory this file is in
        std::string dir = mydirname(midFile);

        soundffile = dir + "/"; // the directory this soundfont might be in
        soundffile += mybasename(dir); // bare name of that soundfont
        soundffile += e; // extension

        //     if(fs::exists(dir + e))
        if (myExists(soundffile))
        {
            return Nullable<std::string>(soundffile);
        }
    }
    
    return Nullable<std::string>(nullptr);
}

bool PageLockMemory(void* ptr, size_t bytes)
{
#if defined(_POSIX_SOURCE)
    return mlock(ptr, bytes) == 0;
#else
    return false;
#endif
}

void PageUnlockMemory(void* ptr, size_t bytes)
{
#if defined(_POSIX_SOURCE)
    munlock(ptr, bytes);
#endif
}
