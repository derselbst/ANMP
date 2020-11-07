#include "Config.h"
#include "AtomicWrite.h"
#include "Common.h"

#include <cereal/archives/json.hpp>
#include <fstream>
#include <filesystem>

// definitions go here
constexpr frame_t Config::FramesToRender;
constexpr const char Config::UserDir[];
constexpr const char Config::UserFile[];

Config &gConfig = gConfig.Singleton();


Config::Config()
{
}

Config &Config::Singleton()
{
    // guaranteed to be destroyed
    static Config instance;

    return instance;
}

void Config::Load() noexcept
{
    string file = ::myHomeDir() + "/" + this->UserDir + "/" + this->UserFile;

    if (::myExists(file))
    {
        try
        {
            std::ifstream is(file);
            cereal::JSONInputArchive ar(is);
            ar(*this);
        }
        catch (...)
        {
            CLOG(LogLevel_t::Error, "Something went wrong when reading settings (might contain garbage?)");
        }
    }
    else
    {
        CLOG(LogLevel_t::Info, "No user settings file does exist");
    }
}

void Config::Save() noexcept
{
    string file = ::myHomeDir() + "/" + this->UserDir + "/";

    if (!::myExists(file))
    {
        std::filesystem::create_directory(file);
    }

    file += this->UserFile;
    try
    {
        std::ofstream os(file);

        if (!os.good())
        {
            throw - 1;
        }

        cereal::JSONOutputArchive ar(os);
        ar(*this);
    }
    catch (...)
    {
        CLOG(LogLevel_t::Error, "Something went wrong when writing settings (no permission?)");
    }
}