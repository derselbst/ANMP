#include "Config.h"

constexpr frame_t Config::FramesToRender; // definition goes here

Config& gConfig = gConfig.Singleton();


Config::Config()
{}

Config& Config::Singleton()
{
    // guaranteed to be destroyed
    static Config instance;

    return instance;
}