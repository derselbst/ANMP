
#include "StringFormatter.h"
#include "Common.h"
#include "Song.h"
#include "SongInfo.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

StringFormatter::StringFormatter()
{
}

StringFormatter &StringFormatter::Singleton()
{
    // guaranteed to be destroyed
    static StringFormatter instance;

    return instance;
}

std::string StringFormatter::GetLastWinError()
{
    unsigned long code = 0;
#ifdef WIN32
    code = GetLastError();
#endif
    return StringFormatter::GetLastWinError(code);
}

std::string StringFormatter::GetLastWinError(unsigned long code)
{
#ifdef WIN32
    constexpr size_t MaxErrorLen = 2048;
#ifdef _UNICODE
    TCHAR err[MaxErrorLen];
    static char ascii_err[MaxErrorLen];
#else
    static TCHAR err[MaxErrorLen];
#endif

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                      nullptr,
                      code,
                      MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                      err,
                      sizeof(err) / sizeof(err[0]),
                      nullptr);

#ifdef UNICODE
        size_t const nLen = wcslen(err);
#else
        size_t const nLen = strlen(err);
#endif
        if (nLen > 1 && err[nLen - 1] == '\n')
        {
            err[nLen - 1] = 0;
            if (err[nLen - 2] == '\r')
            {
                err[nLen - 2] = 0;
            }
        }

#ifdef _UNICODE
        WideCharToMultiByte(CP_UTF8, 0, err, -1, ascii_err, sizeof(ascii_err) / sizeof(ascii_err[0]), 0, 0);
        return ascii_err;
#else
        return err;
#endif
#else
    return "";
#endif
}

void StringFormatter::UpdateReplacements(const SongInfo &info)
{
    if (!this->wildcards.empty())
    {
        this->wildcards.erase(this->wildcards.begin(), this->wildcards.end());
    }
    this->wildcards.reserve(10);

    wildcard_t wild;
    wild.wildcard = "%album%";
    wild.replacement = &info.Album;
    this->wildcards.push_back(wild);

    wild.wildcard = "%genre%";
    wild.replacement = &info.Genre;
    this->wildcards.push_back(wild);

    wild.wildcard = "%title%";
    wild.replacement = &info.Title;
    this->wildcards.push_back(wild);

    wild.wildcard = "%artist%";
    wild.replacement = &info.Artist;
    this->wildcards.push_back(wild);

    wild.wildcard = "%track%";
    wild.replacement = &info.Track;
    this->wildcards.push_back(wild);

    wild.wildcard = "%composer%";
    wild.replacement = &info.Composer;
    this->wildcards.push_back(wild);

    wild.wildcard = "%year%";
    wild.replacement = &info.Year;
    this->wildcards.push_back(wild);
}

string StringFormatter::GetFilename(const Song *song, const std::string& extension)
{
    std::string filename;
    if (!this->pattern.empty())
    {
        this->UpdateReplacements(song->Metadata);

        filename = this->pattern;

        for (size_t i = 0; i < this->wildcards.size(); i++)
        {
            // find placeholder
            size_t pos = filename.find(this->wildcards[i].wildcard);
            if (pos != std::string::npos)
            {
                filename.replace(pos, this->wildcards[i].wildcard.size(), this->wildcards[i].replacement->c_str());
            }
        }

        std::string path = mydirname(song->Filename);

        filename = path + "/" + filename + extension;
    }
    else
    {
        filename = song->Filename + extension;
    }

    return ::getUniqueFilename(filename);
}

void StringFormatter::SetFormat(std::string pattern) noexcept
{
    this->pattern = std::move(pattern);
}
