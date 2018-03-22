
#include <cereal/archives/json.hpp>
#include <fstream>
#include <iostream>

#include "Config.h"
#include "Test.h"

using namespace std;

int main()
{
    {
        std::ofstream os("/home/tom/Eigene Programme/ANMP/build/config.json");
        cereal::JSONOutputArchive ar(os);
        ar(gConfig);
    }
    /*
    {
        std::ifstream is("config.json");
        cereal::JSONInputArchive archive(is);
        gConfig.serialize(ar);
    }*/

    return 0;
}
