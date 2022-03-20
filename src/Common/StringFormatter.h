
#pragma once

#include <string>
#include <vector>

using std::string;

class Song;
struct SongInfo;

class StringFormatter
{
    // Constructors/Destructors
    //

    StringFormatter();

    // no copy
    StringFormatter(const StringFormatter &) = delete;
    // no assign
    StringFormatter &operator=(const StringFormatter &) = delete;

    std::string pattern;

    void UpdateReplacements(const SongInfo &info);

    public:
    struct wildcard_t
    {
        std::string wildcard;
        const std::string *replacement;
    };
    std::vector<StringFormatter::wildcard_t> wildcards;


    static StringFormatter &Singleton();

    /**
     * based on the pattern set by SetFormat(), returns a unique filename suitable for exporting the given Song to
     * 
     * if no such pattern was specified, returns a unique std::string based on Song->Filename
     */
    std::string GetFilename(const Song *song, const std::string& extension = "");

    /**
     * sets the format pattern to be applied for GetFilename()
     */
    void SetFormat(std::string pattern) noexcept;
};
