
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
    StringFormatter(const StringFormatter&) = delete;
    // no assign
    StringFormatter& operator=(const StringFormatter&) = delete;

    string pattern;
    
    void UpdateReplacements(const SongInfo& info) noexcept;
    
public:
    
    struct wildcard_t
    {
        string wildcard;
        const string* replacement;
    };    
    std::vector<StringFormatter::wildcard_t> wildcards;

    
    static StringFormatter& Singleton();
    
    /**
     * based on the pattern set by SetFormat(), returns a unique filename suitable for exporting the given Song to
     * 
     * if no such pattern was specified, returns a unique string based on Song->Filename
     */
    string GetFilename(const Song* song, string extension="") noexcept;
    
    /**
     * sets the format pattern to be applied for GetFilename()
     */
    void SetFormat(string pattern) noexcept;

};
