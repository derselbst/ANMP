
#include "StringFormatter.h"
#include "SongInfo.h"
#include "Song.h"
#include "Common.h"

StringFormatter::StringFormatter()
{}

StringFormatter& StringFormatter::Singleton()
{
    // guaranteed to be destroyed
    static StringFormatter instance;

    return instance;
}

void StringFormatter::UpdateReplacements(const SongInfo& info) noexcept    
{
    if(!this->wildcards.empty())
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

string StringFormatter::GetFilename(const Song* song, string extension) noexcept
{
    string filename;
    if(!this->pattern.empty())
    {
        this->UpdateReplacements(song->Metadata);
        
        filename = this->pattern;

        for(size_t i = 0; i<this->wildcards.size(); i++)
        {
            // find placeholder
            size_t pos = filename.find(this->wildcards[i].wildcard);
            if(pos != string::npos)
            {
                filename.replace(pos, this->wildcards[i].wildcard.size(), this->wildcards[i].replacement->c_str());
            }
        }
        
        string path = mydirname(song->Filename);
        
        filename = path + "/" + filename + extension;
    }
    else
    {
        filename = song->Filename + extension;
    }
    
    return ::getUniqueFilename(filename);
}

void StringFormatter::SetFormat(string pattern) noexcept
{
    this->pattern = std::move(pattern);
}
    