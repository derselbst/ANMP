#include "Common.h"


#include <iostream>
#include <cctype>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>

#ifdef _POSIX_VERSION
#include <strings.h>
#endif

using namespace std;

#ifndef _POSIX_VERSION
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
#ifdef _POSIX_VERSION
    return strncasecmp(str1.c_str(), str2.c_str(), min(str1.size(),str2.size())) == 0;
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
 * @param[in] input: a string in the format of mm:ss
 *            where mm=minutes and ss=seconds
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

    return ssTime.str();
}

/**
 * @param[in] ms: position in audiofile in milliseconds
 * @param[in] sampleRate: sample rate of audiofile in HZ
 *
 * @return position in audiofile in frames (i.e. samples)
 */
frame_t msToFrames(const size_t& ms, const unsigned int& sampleRate)
{
    return (ms*sampleRate)/1000;
}
